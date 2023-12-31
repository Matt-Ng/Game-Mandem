import requests
import json
from playwright.sync_api import sync_playwright
from bs4 import BeautifulSoup

# Creds: izik1 for the optable: https://izik1.github.io/gbops/index.html

p = sync_playwright().start()
browser = p.chromium.launch()
page = browser.new_page()

URL = "https://izik1.github.io/gbops/index.html"
page.goto(URL)

soup = BeautifulSoup(page.content(), "html.parser")

unprefixedtable = soup.find(id="unprefixed-16-t")

tablebody = soup.find('tbody')

unimplementedOpcodes = set()

rows = tablebody.find_all('tr')
operations = []
i = 0
for row in rows:
    cells = row.find_all('td')
    for cell in cells:
        curr = cell.find('div')
        if curr:
            contents = curr.find('div').contents
            print(str(i), contents)
            print("len:", len(contents))
            print("-----")

            code = "0x{:02x}".format(i)
            command = contents[0].split(" ")
            info = contents[2].split(" ")
            flags = contents[4].split('\u200b')

            mnemonic = command[0]
            params = command[1].split(',') if len(command) > 1 else None
            length = info[0]
            timing = info[1]

            operations.append(
                {
                    "code": code,
                    "mnemonic": mnemonic,
                    "args": params,
                    "length": length,
                    "timing": timing,
                    "flags": flags,
                }
            )
            unimplementedOpcodes.add(mnemonic)
        i += 1

print(operations)

#archive the json in case the website goes down

with open('opcodeTable.json', 'w') as f:
    f.write(json.dumps(operations))

def parseLD(arg1, arg2, operation):
    res = ""
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter)",
        "u16": "memory->readWord(programCounter)",
        "SP+i8": "StackPointer.reg + (int8_t) memory->readByte(programCounter)"
    }
    lhs, rhs = "", ""
    bit = 8

    if (len(arg1.strip("()+-")) == 2 or "u16" in arg1) and (len(arg2.strip("()+-")) == 2 or "u16" in arg2):
        bit = 16

    writeToAddress = False
    pcIncrement = 0
    #lhs
    if arg1 in registerDes:
        lhs = registerDes[arg1]
    elif '(' in arg1:
        writeToAddress = True
        if "FF00" in arg1:
            sides = arg1.strip("()").split("+")
            lhs = f"0xFF00 + {registerDes[sides[1]]}"
        elif '+' in arg1:
            # ie; LD (HL+), A -> LHS = RegHL.reg++
            lhs = f"{registerDes[arg1.strip('()+-')]}++"
        elif '-' in arg1:
            lhs = f"{registerDes[arg1.strip('()+-')]}--"
        else:
            lhs = registerDes[arg1.strip("()")]
    
    #rhs
    if arg2 in registerDes:
        rhs = registerDes[arg2]
    elif '(' in arg2:
        if "FF00" in arg2:
            sides = arg2.strip("()").split("+")
            rhs = f"0xFF00 + {registerDes[sides[1]]}"
        elif '+' in arg2:
            # ie; LD (HL+), A -> LHS = RegHL.reg++
            rhs = f"{registerDes[arg2.strip('()+-')]}++"
        elif '-' in arg2:
            rhs = f"{registerDes[arg2.strip('()+-')]}--"
        else:
            if arg2.strip("()") in ["u8", "u16"]:
                rhs = "programCounter"
            else:
                rhs = registerDes[arg2.strip("()")]
        
        if bit == 8:
            rhs = f"memory->readByte({rhs})"
        elif bit == 16:
            rhs = f"memory->readWord({rhs})"

    if "u8" in arg1 or "u8" in arg2 or "i8" in arg2:
        pcIncrement = 1
    elif "u16" in arg1 or "u16" in arg2:
        pcIncrement = 2

    if writeToAddress:
        if bit == 8:
            res = f"\t\tmemory->writeByte({lhs}, {rhs});\n"
        elif bit == 16:
            res = f"\t\tmemory->writeWord({lhs}, {rhs});\n"
    else:
        res = f"\t\t{lhs} = {rhs};\n"


    if pcIncrement == 1:
        res += "\t\tprogramCounter++;\n"
    elif pcIncrement == 2:
        res += "\t\tprogramCounter += 2;\n"
    return res

def parseArith(arg1, arg2, isPlus, operation):
    res = ""
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",  
        "(HL)": "memory->readByte(RegHL.reg)",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter++)",
        "i8": "(int8_t) memory->readByte(programCounter++)"
    }

    bit = 8

    if (len(arg1) == 2):
        bit = 16

    if arg2 == "u8":
        res += f"\t\tuint8_t data = {registerDes[arg2]};\n"
    elif arg2 == "i8":
        res += f"\t\tint8_t data = {registerDes[arg2]};\n"

    if isPlus:
        res += f"\t\t{registerDes[arg1]} += {'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]};\n"
    else:
        res += f"\t\t{registerDes[arg1]} -= {registerDes[arg2]};\n"

    res += flags(registerDes[arg1], operation['flags'], bit, [registerDes[arg1], 'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]])

    return res

def parseOne(arg, isPlus, operation):
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",
        "(HL)": "memory->readByte(RegHL.reg)",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter++)",
        "i8": "(int8_t) memory->readByte(programCounter++)"
    }

    bit = 8
    if (len(arg) == 2):
        bit = 16

    res = ""

    curr = arg
    if "(" in arg:
        regi = arg.strip('()')
        if len(arg.strip("()")) == 2:
            curr = f"memory->writeWord(memory->readWord({registerDes[regi]}), {registerDes[regi]}"
        else:
            curr = f"memory->writeByte(memory->readByte({registerDes[regi]}), {registerDes[regi]}"
        if isPlus:
            res += f"\t\t{curr} + 1);\n"
        else:
            res += f"\t\t{curr} - 1);\n"
    else:
        curr = registerDes[curr]
        if isPlus:
            res += f"\t\t{curr}++;\n"
        else:
            res += f"\t\t{curr}--;\n"
        
    res += flags(registerDes[arg], operation['flags'], bit, [registerDes[arg], "1"])
    return res

def parseCarryArith(arg1, arg2, isPlus, operation):
    res = ""
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",  
        "(HL)": "memory->readByte(RegHL.reg)",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter++)",
        "i8": "(int8_t) memory->readByte(programCounter++)"
    }

    bit = 8

    if arg2 == "u8":
        res += f"\t\tuint8_t data = {registerDes[arg2]};\n"
    elif arg2 == "i8":
        res += f"\t\tint8_t data = {registerDes[arg2]};\n"

    res += "\t\tuint8_t carry = getFlag(FLAG_C);\n"

    if isPlus:
        res += f"\t\t{registerDes[arg1]} += ({'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]} + carry);\n"
    else:
        res += f"\t\t{registerDes[arg1]} -= ({'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]} - carry);\n"
    res += flags(registerDes[arg1], operation['flags'], bit, [registerDes[arg1], 'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]])
    return res

def parseLogic(arg1, arg2, operation):
    res = ""
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",  
        "(HL)": "memory->readByte(RegHL.reg)",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter++)",
        "i8": "(int8_t) memory->readByte(programCounter++)"
    }

    bit = 8
    if arg2 == "u8":
        res += f"\t\tuint8_t data = {registerDes[arg2]};\n"
    elif arg2 == "i8":
        res += f"\t\tint8_t data = {registerDes[arg2]};\n"

    logicalOperator = operation['mnemonic']

    if logicalOperator == "AND":
        res += f"\t\t{registerDes[arg1]} &= {'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]};\n"
    elif logicalOperator == "OR":
        res += f"\t\t{registerDes[arg1]} |= {'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]};\n"
    elif logicalOperator == "XOR":
        res += f"\t\t{registerDes[arg1]} ^= {'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]};\n"

    res += flags(registerDes[arg1], operation['flags'], bit, [registerDes[arg1], 'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]])

    return res

def parseCompare(arg1, arg2, operation):
    res = ""
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",  
        "(HL)": "memory->readByte(RegHL.reg)",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter++)",
        "i8": "(int8_t) memory->readByte(programCounter++)"
    }
    bit = 8

    if arg2 == "u8":
        res += f"\t\tuint8_t data = {registerDes[arg2]};\n"
    elif arg2 == "i8":
        res += f"\t\tint8_t data = {registerDes[arg2]};\n"

    res += f"\t\tuint8_t result = {registerDes[arg1]} - {'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]};\n"

    res += flags(registerDes[arg1], operation['flags'], bit, ["result", 'data' if arg2 in ['u8', 'i8'] else registerDes[arg2]])
    return res

def parseJP(operation):
    res = ""
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",
        "(HL)": "memory->readByte(RegHL.reg)",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter++)",
        "u16": "memory->readWord(programCounter)",
        "i8": "(int8_t) memory->readByte(programCounter++)",
    }

    flags = {
        "NZ": "!getFlag(FLAG_Z)",
        "Z": "getFlag(FLAG_Z)",
        "NC": "!getFlag(FLAG_C)",
        "C": "getFlag(FLAG_C)",
    }

    if len(operation['args']) == 1:
        res += f"\t\tprogramCounter = {registerDes[operation['args'][0]]};\n"
    if len(operation['args']) == 2:
        res += f"\t\tuint16_t newAddress = {registerDes[operation['args'][1]]};\n"
        res += "\t\tprogramCounter += 2;\n"
        res += f"\t\tif({flags[operation['args'][0]]})\u007b\n"
        res += "\t\t\tprogramCounter = newAddress;\n"
        res += "\t\t\u007d\n"
    return res

def parseJR(operation):
    res = ""
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",
        "(HL)": "memory->readByte(RegHL.reg)",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter++)",
        "u16": "memory->readWord(programCounter)",
        "i8": "(int8_t) memory->readByte(programCounter++)",
    }

    flags = {
        "NZ": "!getFlag(FLAG_Z)",
        "Z": "getFlag(FLAG_Z)",
        "NC": "!getFlag(FLAG_C)",
        "C": "getFlag(FLAG_C)",
    }
    
    if len(operation['args']) == 1:
        res += f"\t\tint8_t jumpBy = {registerDes[operation['args'][0]]};\n"
        res += f"\t\tprogramCounter += jumpBy;\n"
    if len(operation['args']) == 2:
        res += f"\t\tint8_t jumpBy = {registerDes[operation['args'][1]]};\n"
        res += f"\t\tif({flags[operation['args'][0]]})\u007b\n"
        res += f"\t\t\tprogramCounter += jumpBy;\n"
        res += "\t\t\u007d\n"
    return res

def parsePop(operation):
    res = ""
    registerDes = {
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",
    }

    bit = 16

    arg = operation['args'][0]
    res += f"\t\t{registerDes[arg]} = memory->readWord(StackPointer.reg);\n"
    if arg == "AF":
        res += "\t\t// lower nibble of F register must be reset\n"
        res += "\t\tRegAF.lo &= 0xF0;\n"

    return res

def parsePush(operation):
    res = ""

    registerDes = {
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",
    }

    arg = operation['args'][0]
    res += "\t\tStackPointer.reg--;\n"
    res += f"\t\tmemory->writeWord(StackPointer.reg, {registerDes[arg]});\n"
    res += "\t\tStackPointer.reg--;\n"
    return res

def parseRet(operation):
    res = ""

    flags = {
        "NZ": "!getFlag(FLAG_Z)",
        "Z": "getFlag(FLAG_Z)",
        "NC": "!getFlag(FLAG_C)",
        "C": "getFlag(FLAG_C)",
    }

    if not operation['args']:
        res += "\t\tprogramCounter = memory->readWord(StackPointer.reg);\n"
        res += "\t\tStackPointer.reg += 2;\n"
    else:
        res += f"\t\tif({flags[operation['args'][0]]})\u007b\n"
        res += "\t\t\tprogramCounter = memory->readWord(StackPointer.reg);\n"
        res += "\t\t\tStackPointer.reg += 2;\n"
        res += "\t\t\u007d\n"
    return res

def parseRotate(operation):
    res = ""
    if operation['mnemonic'] == "RRA":
        res += "\t\trr(RegAF.hi);\n"
    elif operation['mnemonic'] == "RLA":
        res += "\t\trl(RegAF.hi);\n"
    elif operation['mnemonic'] == "RRCA":
        res += "\t\trr(RegAF.hi);\n"
        res += "\t\t// make msb equal to carry bit"
        res += "\t\tRegAF.hi |= ((0x80) & (7 << getFlag(FLAG_C)));\n"
    elif operation['mnemonic'] == "RLCA":
        res += "\t\trl(RegAF.hi);\n"
        res += "\t\t// make lsb equal to carry bit"
        res += "\t\tRegAF.lo |= ((0x01) & (getFlag(FLAG_C)));\n"
    return res

def parseCall(operation):
    res = ""
    registerDes = {
        "A": "RegAF.hi",
        "F": "RegAF.lo",
        "B": "RegBC.hi",
        "C": "RegBC.lo",
        "D": "RegDE.hi",
        "E": "RegDE.lo",
        "H": "RegHL.hi",
        "L": "RegHL.lo",
        "AF": "RegAF.reg",
        "BC": "RegBC.reg",
        "DE": "RegDE.reg",
        "HL": "RegHL.reg",
        "(HL)": "memory->readByte(RegHL.reg)",
        "SP": "StackPointer.reg",
        "u8": "memory->readByte(programCounter++)",
        "u16": "memory->readWord(programCounter)",
        "i8": "(int8_t) memory->readByte(programCounter++)",
    }

    flags = {
        "NZ": "!getFlag(FLAG_Z)",
        "Z": "getFlag(FLAG_Z)",
        "NC": "!getFlag(FLAG_C)",
        "C": "getFlag(FLAG_C)",
    }

    if len(operation['args']) == 1:
        res += f"\t\tuint16_t newAddress = {registerDes[operation['args'][0]]};\n"
        res += "\t\tprogramCounter += 2;\n"
        res += "\t\tStackPointer.reg--;\n"
        res += "\t\tmemory->writeWord(StackPointer.reg, programCounter);\n"
        res += "\t\tprogramCounter = newAddress;\n"
    else:
        res += f"\t\tuint16_t newAddress = {registerDes[operation['args'][1]]};\n"
        res += "\t\tprogramCounter += 2;\n"
        res += f"\t\tif({flags[operation['args'][0]]})\u007b\n"
        res += "\t\t\tStackPointer.reg--;\n"
        res += f"\t\t\tmemory->writeWord(StackPointer.reg, programCounter);\n"
        res += "\t\t\tprogramCounter = newAddress;\n"
        res += "\t\t\u007d\n"
    return res

def flags(resSymbol, flags, bit, args = ["AHHHHHHHH", "AHHHHHHH"]):
    # flags go in this order: Z, N, H, C
    flagCode = ""
    if flags[0] == 'Z':
        flagCode += f"\t\tif({resSymbol} == 0)\u007b\n"
        flagCode += f"\t\t\tsetFlag(FLAG_Z, 1);\n"
        flagCode += f"\t\t\u007d\n"
    elif flags[0] == '0' or flags[0] == '1':
        flagCode += f"\t\tsetFlag(FLAG_Z, {flags[0]});\n"
    
    if flags[1] != "-":
        flagCode += f"\t\tsetFlag(FLAG_N, {flags[1]});\n"
    
    if flags[2] == 'H':
        flagCode += f"\t\tsetFlag(FLAG_H, halfCarry{str(bit)}({args[0]}, {args[1]}));\n"
    elif flags[2] == '0' or flags[2] == '1':
        flagCode += f"\t\tsetFlag(FLAG_H, {flags[2]});\n"
    
    if flags[3] == "C":
        flagCode += f"\t\tsetFlag(FLAG_C, {resSymbol} > {'0xff' if bit == 8 else '0xffff'});\n"
    elif flags[3] == '0' or flags[3] == '1':
        flagCode += f"\t\tsetFlag(FLAG_C, {flags[3]});\n"
    return flagCode

opcodestr = "switch(opCode){\n"

for operation in operations:
    print(operation)
    curr = f"\tcase {operation['code']}: \u007b\n" 
    curr += f"\t\t// {operation['mnemonic']} {', '.join(operation['args']) if operation['args'] else ''}\n"
    curr += f"\t\t// Flags: {''.join(operation['flags'])}\n" if operation['flags'] != ['-', '-', '-', '-'] else ""
    if operation['mnemonic'] == "LD":
       arg1 = operation['args'][0]
       arg2 = operation['args'][1]
       curr += parseLD(arg1, arg2, operation)
       if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "ADD":
        curr += parseArith(operation['args'][0], operation['args'][1], True, operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "SUB":
        curr += parseArith(operation['args'][0], operation['args'][1], False, operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "INC":
        curr += parseOne(operation['args'][0], True, operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "DEC":
        curr += parseOne(operation['args'][0], False, operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "ADC":
        curr += parseCarryArith(operation['args'][0], operation['args'][1], True, operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "SBC":
        curr += parseCarryArith(operation['args'][0], operation['args'][1], False, operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] in ['AND', 'OR', 'XOR']:
        curr += parseLogic(operation['args'][0], operation['args'][1], operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "CP":
        curr += parseCompare(operation['args'][0], operation['args'][1], operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "SCF":
        curr += flags("", operation['flags'], 8)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "CCF":
        curr += "\t\tsetFlag(FLAG_N, 0);\n"
        curr += "\t\tsetFlag(FLAG_H, 0);\n"
        curr += "\t\tsetFlag(FLAG_C, !getFlag(FLAG_C));\n"
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "JP":
        curr += parseJP(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "JR":
        curr += parseJR(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "POP":
        curr += parsePop(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "PUSH":
        curr += parsePush(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "CPL":
        curr += "\t\tRegAF.hi = ~RegAF.hi;\n"
        curr += "\t\tsetFlag(FLAG_N, 1);\n"
        curr += "\t\tsetFlag(FLAG_H, 1);\n"
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "RET":
        curr += parseRet(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] in ['RRA', "RLA", "RRCA", "RLCA"]:
        curr += parseRotate(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "DAA":
        curr += """\t\tuint8_t AReg = RegAF.hi;
        uint8_t offset = 0;
        if((!getFlag(FLAG_N) && (AReg & 0xF) > 0x09) || getFlag(FLAG_H)){
        \toffset |= 0x06;
        }
        if((!getFlag(FLAG_N) && (AReg & 0xF) > 0x99) || getFlag(FLAG_C)){
        \toffset |= 0x60;
        \tsetFlag(FLAG_C, 1);
        }
        if(!getFlag(FLAG_N)){
        \tRegAF.hi += offset;
        }
        else{
        \tRegAF.hi -= offset;
        }
        setFlag(FLAG_Z, RegAF.hi ? 0 : 1);
        setFlag(FLAG_H, 0);\n"""
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "CALL":
        curr += parseCall(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    elif operation['mnemonic'] == "RST":
        curr += f"\t\tuint8_t val = memory->readByte({operation['code']});\n"
        curr += f"\t\tStackPointer.reg--;\n"
        curr += f"\t\tmemory->writeByte(StackPointer.reg--, programCounter >> 8);\n"
        curr += f"\t\tmemory->writeByte(StackPointer.reg, programCounter & 0xff);\n"
        curr += "\t\tprogramCounter = 0;\n"
        curr += "\t\tprogramCounter |= val;\n"
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    curr += f'\t\tstd::cout<<"{operation["mnemonic"]} {", ".join(operation["args"]) if operation["args"] else ""}"<<std::endl;\n'
    curr += "\t\u007d"
    curr+="\t\tbreak;\n"
    opcodestr += curr
opcodestr += "\tdefault:\n\t\tstd::cout << \"invalid or unimplemented op code\" << std::endl;\n\t\tbreak;\n"
opcodestr += "}"

print(opcodestr)

with open('opcode.txt', 'w') as f:
    f.write(opcodestr)

excludefornow = ["NOP", "HALT", "STOP", "DI", "EI", "RETI", "PREFIX"]

for op in excludefornow:
    unimplementedOpcodes.remove(op)

print("unimplemented opcodes:", "\n".join(unimplementedOpcodes))