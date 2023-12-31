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

prefixedtable = soup.find(id="cbprefixed-16-t")

tablebody = prefixedtable.find('tbody')

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


def parseStuff(operation):
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
    arg = operation['args'][0] if operation['args'] else ""
    if arg == "(HL)":
        res += f"\t\tuint8_t hlVal = {registerDes[arg]};\n"
        registerDes[arg] = "hlVal"

    res += f"\t\t{operation['mnemonic'].lower()}({registerDes[operation['args'][0]] if operation['args'] else ''});\n"
    
    if arg == "(HL)":
        res += "\t\tmemory->writeByte(RegHL.reg, hlVal);\n"
    
    return res

def parseBit(operation):
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
    arg1 = operation['args'][0]
    arg2 = operation['args'][1]
    if arg2 == "(HL)":
        res += f"\t\tuint8_t hlVal = {registerDes[arg2]};\n"
        registerDes[arg2] = "hlVal"

    res += f"\t\t{operation['mnemonic'].lower()}({arg1}, {registerDes[arg2]});\n"

    if operation['mnemonic'] != "bit" and arg2 == "(HL)":
        res += "\t\tmemory->writeByte(RegHL.reg, hlVal);\n"
    
    return res

print(operations)

opcodestr = "switch(opCode){\n"

for operation in operations:
    curr = f"\tcase {operation['code']}: \u007b\n" 
    curr += f"\t\t// {operation['mnemonic']} {', '.join(operation['args']) if operation['args'] else ''}\n"
    curr += f"\t\t// Flags: {''.join(operation['flags'])}\n" if operation['flags'] != ['-', '-', '-', '-'] else ""
    if operation['mnemonic'] in ["RLC", "RL", "RR", "RRC"]:
        curr += parseStuff(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    if operation['mnemonic'] in ["SLA", "SRA", "SRL", "SWAP"]:
        curr += parseStuff(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    if operation['mnemonic'] in ["BIT", "SET", "RES"]:
        curr += parseBit(operation)
        if operation['mnemonic'] in unimplementedOpcodes:
           unimplementedOpcodes.remove(operation['mnemonic'])
    
    curr += f'\t\tstd::cout<<"{operation["mnemonic"]} {", ".join(operation["args"]) if operation["args"] else ""}"<<std::endl;\n'
    curr += "\t\u007d\n"
    curr+="\tbreak;\n"
    opcodestr += curr
opcodestr += "\tdefault:\n\t\tstd::cout << \"invalid or unimplemented op code\" << std::endl;\n\t\tbreak;\n"
opcodestr += "}"


print(opcodestr)

with open('prefixopcode.txt', 'w') as f:
    f.write(opcodestr)
print("unimplemented opcodes:", "\n".join(unimplementedOpcodes))