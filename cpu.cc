#include <iostream>
#include <fstream>
#include <sstream>

#include "cpu.hh"

/**
 * @brief 
 * Emulate the internal state of the gameboy Z80
 */

CPU::CPU(Memory *memory, Interrupt *interrupt, Timer *timer){
    programCounter = 0x100;
    RegAF.reg = 0x01B0;
    RegBC.reg = 0x0013;
    RegDE.reg = 0x00D8;
    RegHL.reg = 0x014D;

    StackPointer.reg = 0xFFFE;

    this->memory = memory;
    this->interrupt = interrupt;
    this->timer = timer;
}

void CPU::debugPrint(std::string str){
    if(debugMode){
        std::cout << "\t" << str << std::endl;
    }
}

void CPU::toggleDebugMode(bool val){
    debugMode = val;
}

void CPU::test(){
    std::ifstream GB_ROM("DMG_ROM.bin", std::ios::binary);
    GB_ROM.seekg(0, std::ios::end);
    int fileSize = GB_ROM.tellg();
    GB_ROM.seekg(0, std::ios::beg);

    uint8_t cartridgeFile[fileSize];
    GB_ROM.read((char *)cartridgeFile, fileSize);

    for(int i = 0; i < fileSize; i++){
        executeOP(cartridgeFile[i]);
    }
    printf("filesize: %d\n", fileSize);
}

uint8_t CPU::getFlag(uint8_t flag){
    return (FLAGS & (1 << flag)) >> flag;
}

void CPU::setFlag(uint8_t flag, uint8_t val){
    FLAGS = val ? (FLAGS | (1 << flag)) : (FLAGS & ~(1 << flag));
}

uint8_t CPU::halfCarry8(uint8_t a, uint8_t b){
    return (((a & 0xf) + (b & 0xf)) & 0x10) == 0x10;
}

uint8_t CPU::halfCarry8(uint8_t a, uint8_t b, uint8_t res){
    return (a ^ b ^ res) & 0x10;
}

uint8_t CPU::halfCarry16(uint16_t a, uint16_t b){
    return (((a & 0xfff) + (b & 0xfff)) & 0x1000) == 0x1000;
}

uint8_t CPU::halfCarry16(uint16_t a, uint16_t b, uint16_t res){
    return (a ^ b ^ res) & 0x1000;
}

void CPU::resetFlags(){
    RegAF.lo = 0;
}

void CPU::add8(uint8_t &a, uint8_t b){
    uint16_t res = a + b;

    
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, halfCarry8(a, b));
    setFlag(FLAG_C, res > 0xff);

    a += b;
    setFlag(FLAG_Z, !a);
}

void CPU::add16(uint16_t &a, uint16_t b){
    uint32_t res = a + b;
    
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, halfCarry16(a, b));
    setFlag(FLAG_C, res > 0xffff);

    a += b;
}

void CPU::adc(uint8_t &a, uint8_t b){
    uint8_t carry = getFlag(FLAG_C);
    uint16_t res = a + b + carry;

    setFlag(FLAG_N, 0);
    // do half carry in function to account for possible b + carry overflow
    setFlag(FLAG_H, (((a & 0xf) + (b & 0xf) + (carry & 0xf)) & 0x10) == 0x10);
    setFlag(FLAG_C, res > 0xff);

    a += b + carry;
    setFlag(FLAG_Z, !a);
}

void CPU::add8Signed(uint8_t &a, int b){
    uint16_t res = a + b;
    
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, (((a & 0xf) + (b)) & 0x10) == 0x10);

    // either the addition overflows or underflows
    setFlag(FLAG_C, (res > 0xff) || (b < 0 && (b * -1) > a));

    a += b;
    setFlag(FLAG_Z, !a);
}

void CPU::add16Signed(uint16_t &a, int b){
    uint32_t res = a + b;

    // this actually performs 8bit arithmetic on a 16 bit unsigned
    setFlag(FLAG_N, 0);
    // checking for a half 16 bit carry will discern an 8 bit arith carry
    setFlag(FLAG_H, halfCarry8(a, b, res & 0xFFFF));

    a += b;
    setFlag(FLAG_Z, !a);
}

void CPU::sub(uint8_t &a, uint8_t b){
    setFlag(FLAG_N, 1);
    setFlag(FLAG_H, (((a & 0xf) - (b & 0xf)) & 0x10) == 0x10);
    setFlag(FLAG_C, a < b);
    
    a -= b;
    setFlag(FLAG_Z, !a);
}

void CPU::sbc(uint8_t &a, uint8_t b){
    uint8_t carry = getFlag(FLAG_C);
    int res = a - b - carry;

    setFlag(FLAG_N, 1);
    setFlag(FLAG_H, (((a & 0xf) - (b & 0xf) - (carry & 0xf)) & 0x10) == 0x10);
    setFlag(FLAG_C, res < 0);

    a -= b;
    a -= carry;
    setFlag(FLAG_Z, !a);
}

void CPU::inc8(uint8_t &a){
    setFlag(FLAG_H, halfCarry8(a, 1));

    a++;
    setFlag(FLAG_Z, !a);
    setFlag(FLAG_N, 0);
}

void CPU::dec8(uint8_t &a){
    setFlag(FLAG_H, (((a & 0xf) - (1 & 0xf)) & 0x10) == 0x10);

    a--;
    setFlag(FLAG_Z, !a);
    setFlag(FLAG_N, 1);
}

void CPU::cp(uint8_t a, uint8_t b){
    uint8_t result = a - b;
    setFlag(FLAG_Z, !result);
    setFlag(FLAG_N, 1);
    setFlag(FLAG_H, (((a & 0xf) - (b & 0xf)) & 0x10) == 0x10);
    setFlag(FLAG_C, a < b);
}

void CPU::rl(uint8_t &reg){
    uint8_t oldCarry = getFlag(FLAG_C);
    setFlag(FLAG_C, reg & 0x80);

    reg <<= 1;
    reg |= oldCarry;

    setFlag(FLAG_Z, !reg);
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 0);
}


void CPU::rlc(uint8_t &reg){
    setFlag(FLAG_C, reg & 0x80);

    reg <<= 1;
    reg |= getFlag(FLAG_C);

    setFlag(FLAG_Z, !reg);
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 0);
}

void CPU::rla(){
    rl(RegAF.hi);
    setFlag(FLAG_Z, 0);
}

void CPU::rlca(){
    rlc(RegAF.hi);
    setFlag(FLAG_Z, 0);
}

void CPU::rr(uint8_t &reg){
    uint8_t oldCarry = getFlag(FLAG_C);

    setFlag(FLAG_C, reg & 0x01);

    reg >>= 1;
    reg |= (oldCarry << 7);

    setFlag(FLAG_Z, !reg);
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 0);
}

void CPU::rrc(uint8_t &reg){
    setFlag(FLAG_C, reg & 0x01);

    reg >>= 1;
    reg |= (getFlag(FLAG_C) << 7);

    setFlag(FLAG_Z, !reg);
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 0);
}

void CPU::rra(){
    rr(RegAF.hi);
    setFlag(FLAG_Z, 0);
}

void CPU::rrca(){
    rrc(RegAF.hi);
    setFlag(FLAG_Z, 0);
}

void CPU::sla(uint8_t &reg){
    setFlag(FLAG_C, reg & 0x80);
    reg <<= 1;

    setFlag(FLAG_Z, !reg);
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 0);
}

void CPU::sra(uint8_t &reg){
    uint8_t oldMSB = reg & 0x80;
    setFlag(FLAG_C, reg & 0x01);

    reg >>= 1;
    reg |= oldMSB;

    setFlag(FLAG_Z, !reg);
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 0);
}

void CPU::srl(uint8_t &reg){
    setFlag(FLAG_C, reg & 0x01);

    reg >>= 1;
    // set msb to 0
    reg &= 0x7F;

    setFlag(FLAG_Z, !reg);
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 0);
}

void CPU::swap(uint8_t &reg){
    reg = (reg >> 4) | (reg << 4);

    setFlag(FLAG_Z, !reg);
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 0);
    setFlag(FLAG_C, 0);
}

void CPU::bit(uint8_t n, uint8_t reg){
    setFlag(FLAG_Z, !((reg >> n) & 1));
    setFlag(FLAG_N, 0);
    setFlag(FLAG_H, 1);
}

void CPU::set(uint8_t n, uint8_t &reg){
    reg |= (1 << n);
}

void CPU::res(uint8_t n, uint8_t &reg){
    reg &= ~(1 << n);
}

void CPU::handleInterrupts(){
    if(interrupt->IME){
        uint8_t interruptFlag = memory->readByte(INTERRUPT_FLAG);
        uint8_t interruptEnabled = memory->readByte(INTERRUPT_ENABLE);

        // flag must be set and enabled for flag must be set

        // VBLANK
        if (((interruptFlag >> VBLANK) & 1) && ((interruptEnabled >> VBLANK) & 1)){
            interruptServiceRoutine(VBLANK);
        }

        // LCD
        if (((interruptFlag >> VBLANK) & 1) && ((interruptEnabled >> VBLANK) & 1)){
            interruptServiceRoutine(VBLANK);
        }

        // TIMER
        if (((interruptFlag >> VBLANK) & 1) && ((interruptEnabled >> VBLANK) & 1)){
            interruptServiceRoutine(VBLANK);
        }

        // SERIAL
        if (((interruptFlag >> VBLANK) & 1) && ((interruptEnabled >> VBLANK) & 1)){
            interruptServiceRoutine(VBLANK);
        }

        // JOYPAD
        if (((interruptFlag >> VBLANK) & 1) && ((interruptEnabled >> VBLANK) & 1)){
            interruptServiceRoutine(VBLANK);
        }
    }
}

void CPU::interruptServiceRoutine(uint8_t interruptCode){
    // disable interrupts and reset the particular interrupt flag
    interrupt->toggleIME(false);
    uint8_t interruptFlag = memory->readByte(INTERRUPT_FLAG);
    interruptFlag &= ~(1 << interruptCode);
    memory->writeByte(INTERRUPT_FLAG, interruptFlag);

    // push to stack
    memory->writeWord(StackPointer.reg, programCounter);
    StackPointer.reg -= 2;  
    
    if(interruptCode == VBLANK){
        programCounter = 0x40;
    }
    else if(interruptCode == LCD){
        programCounter = 0x48;
    }
    else if(interruptCode == TIMER){
        programCounter = 0x50;
    }
    else if(interruptCode == SERIAL){
        // TODO: handle serial case
        programCounter = 0x58;
    }
    else if(interruptCode == JOYPAD){
        programCounter = 0x60;
    }
}

uint8_t CPU::step(){

    // std::stringstream ss;

    // ss << std::hex << programCounter;

    // std::string str = "executing op at programCounter val 0x" + ss.str();

    std::string str = "";
    std::stringstream ss;

    ss << "PC: 0x" << std::hex << programCounter << " AF: 0x" << std::hex << RegAF.reg << " BC: 0x" << std::hex << RegBC.reg << " DE: 0x" << std::hex << RegDE.reg << " HL: 0x" << std::hex << RegHL.reg << " SP: 0x" << std::hex << StackPointer.reg << " IME 0x" << std::hex << interrupt->IME << std::endl;
    ss << "Flags: Z: " << (int) getFlag(FLAG_Z) << " N: " << (int) getFlag(FLAG_N) << " H: " << (int) getFlag(FLAG_H) << " C: " << (int) getFlag(FLAG_C);
    str += "\n----------------------\n";
    str += ss.str();
    debugPrint(str);
    return executeOP(memory->readByte(programCounter++));
}

uint8_t CPU::executeOP(uint8_t opCode){
    uint8_t time = nonprefixTimings[opCode];

    if(lastInstructionEI){
        interrupt->toggleIME(true);
        lastInstructionEI = false;
    }
    
    switch(opCode){
        case 0x00: {
            // NOP 
            //debugPrint("NOP ");
        }
        break;
        case 0x01: {
            // LD BC, u16
            RegBC.reg = memory->readWord(programCounter);
            programCounter += 2;
            debugPrint("LD BC, u16");
        }
        break;
        case 0x02: {
            // LD (BC), A
            memory->writeByte(RegBC.reg, RegAF.hi);
            debugPrint("LD (BC), A");
        }
        break;
        case 0x03: {
            // INC BC
            RegBC.reg++;
            debugPrint("INC BC");
        }
        break;
        case 0x04: {
            // INC B
            // Flags: Z0H-
            inc8(RegBC.hi);
            debugPrint("INC B");
        }
        break;
        case 0x05: {
            // DEC B
            // Flags: Z1H-
            dec8(RegBC.hi);
            debugPrint("DEC B");
        }
        break;
        case 0x06: {
            // LD B, u8
            RegBC.hi = memory->readByte(programCounter);
            programCounter++;
            debugPrint("LD B, u8");
        }
        break;
        case 0x07: {
            // RLCA 
            // Flags: 000C
            rlca();
            debugPrint("RLCA ");
        }
        break;
        case 0x08: {
            // LD (u16), SP
            uint16_t addr = memory->readWord(programCounter);
            memory->writeWord(addr, StackPointer.reg);
            programCounter += 2;
            debugPrint("LD (u16), SP");
            // printf("arg: 0x%x\n", addr);
        }
        break;
        case 0x09: {
            // ADD HL, BC
            // Flags: -0HC
            add16(RegHL.reg, RegBC.reg);
            debugPrint("ADD HL, BC");
        }
        break;
        case 0x0a: {
            // LD A, (BC)
            RegAF.hi = memory->readByte(RegBC.reg);
            debugPrint("LD A, (BC)");
        }
        break;
        case 0x0b: {
            // DEC BC
            RegBC.reg--;
            debugPrint("DEC BC");
        }
        break;
        case 0x0c: {
            // INC C
            // Flags: Z0H-
            inc8(RegBC.lo);
            debugPrint("INC C");
        }
        break;
        case 0x0d: {
            // DEC C
            // Flags: Z1H-
            dec8(RegBC.lo);
            debugPrint("DEC C");
            // printf("new val: %d\n", RegBC.lo);
        }
        break;
        case 0x0e: {
            // LD C, u8
            RegBC.lo = memory->readByte(programCounter);
            programCounter++;
            debugPrint("LD C, u8");
            // printf("arg: %x\n", RegBC.lo);
        }
        break;
        case 0x0f: {
            // RRCA 
            // Flags: 000C
            rrca();
            debugPrint("RRCA ");
        }
        break;
        case 0x10: {
            // STOP 
            debugPrint("STOP ");
        }
        break;
        case 0x11: {
            // LD DE, u16
            RegDE.reg = memory->readWord(programCounter);
            programCounter += 2;
            debugPrint("LD DE, u16");
            // printf("arg: 0x%x\n", RegDE.reg);
        }
        break;
        case 0x12: {
            // LD (DE), A
            memory->writeByte(RegDE.reg, RegAF.hi);
            debugPrint("LD (DE), A");
        }
        break;
        case 0x13: {
            // INC DE
            RegDE.reg++;
            debugPrint("INC DE");
        }
        break;
        case 0x14: {
            // INC D
            // Flags: Z0H-
            inc8(RegDE.hi);
            debugPrint("INC D");
            //// printf("new Val: %d\n", RegDE.hi);
        }
        break;
        case 0x15: {
            // DEC D
            // Flags: Z1H-
            dec8(RegDE.hi);
            debugPrint("DEC D");
        }
        break;
        case 0x16: {
            // LD D, u8
            RegDE.hi = memory->readByte(programCounter);
            programCounter++;
            debugPrint("LD D, u8");
        }
        break;
        case 0x17: {
            // RLA 
            // Flags: 000C
            rla();
            debugPrint("RLA ");
        }
        break;
        case 0x18: {
            // JR i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            programCounter += jumpBy;
            debugPrint("JR i8");
        }
        break;
        case 0x19: {
            // ADD HL, DE
            // Flags: -0HC
            add16(RegHL.reg, RegDE.reg);
            debugPrint("ADD HL, DE");
        }
        break;
        case 0x1a: {
            // LD A, (DE)
            RegAF.hi = memory->readByte(RegDE.reg);
            debugPrint("LD A, (DE)");
        }
        break;
        case 0x1b: {
            // DEC DE
            RegDE.reg--;
            debugPrint("DEC DE");
        }
        break;
        case 0x1c: {
            // INC E
            // Flags: Z0H-
            inc8(RegDE.lo);
            debugPrint("INC E");
            // printf("new E: %d\n", RegDE.lo);
        }
        break;
        case 0x1d: {
            // DEC E
            // Flags: Z1H-
            dec8(RegDE.lo);
            RegDE.lo--;
            debugPrint("DEC E");
        }
        break;
        case 0x1e: {
            // LD E, u8
            RegDE.lo = memory->readByte(programCounter);
            programCounter++;
            debugPrint("LD E, u8");
        }
        break;
        case 0x1f: {
            // RRA 
            // Flags: 000C
            rra();
            debugPrint("RRA ");
        }
        break;
        case 0x20: {
            // JR NZ, i8
            time += 8;		
            uint8_t jumpBy = memory->readByte(programCounter++);
            if(!getFlag(FLAG_Z)){
                // printf("branched\n");
                programCounter += (int8_t) jumpBy;
                time += 4;
            }
            debugPrint("JR NZ, i8");
            // printf("arg: 0x%x (%d) (signed: %d)\n", jumpBy, jumpBy, (int8_t)jumpBy);
        }
        break;
        case 0x21: {
            // LD HL, u16
            RegHL.reg = memory->readWord(programCounter);
            programCounter += 2;
            debugPrint("LD HL, u16");
            // printf("arg: 0x%x\n", RegHL.reg);
        }
        break;
        case 0x22: {
            // LD (HL+), A
            memory->writeByte(RegHL.reg++, RegAF.hi);
            debugPrint("LD (HL+), A");
        }
        break;
        case 0x23: {
            // INC HL
            RegHL.reg++;
            debugPrint("INC HL");
        }
        break;
        case 0x24: {
            // INC H
            // Flags: Z0H-
            inc8(RegHL.hi);
            debugPrint("INC H");
        }
        break;
        case 0x25: {
            // DEC H
            // Flags: Z1H-
            dec8(RegHL.hi);
            debugPrint("DEC H");
        }
        break;
        case 0x26: {
            // LD H, u8
            RegHL.hi = memory->readByte(programCounter);
            programCounter++;
            debugPrint("LD H, u8");
            // printf("arg: 0x%x\n", RegHL.hi);
        }
        break;
        case 0x27: {
            // DAA 
            // Flags: Z-0C
            uint8_t AReg = RegAF.hi;
            uint8_t offset = 0;
            if((!getFlag(FLAG_N) && (AReg & 0xF) > 0x09) || getFlag(FLAG_H)){
                offset |= 0x06;
            }
            if((!getFlag(FLAG_N) && AReg > 0x99) || getFlag(FLAG_C)){
                offset |= 0x60;
                setFlag(FLAG_C, 1);
            }
            if(!getFlag(FLAG_N)){
                RegAF.hi += offset;
            }
            else{
                RegAF.hi -= offset;
            }
            setFlag(FLAG_Z, RegAF.hi ? 0 : 1);
            setFlag(FLAG_H, 0);
            debugPrint("DAA ");
        }
        break;
        case 0x28: {
            // JR Z, i8
            time += 8;		
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(getFlag(FLAG_Z)){
                programCounter += jumpBy;
                time += 4;
            }
            debugPrint("JR Z, i8");
        }
        break;
        case 0x29: {
            // ADD HL, HL
            // Flags: -0HC
            add16(RegHL.reg, RegHL.reg);
            debugPrint("ADD HL, HL");
        }
        break;
        case 0x2a: {
            // LD A, (HL+)
            RegAF.hi = memory->readByte(RegHL.reg++);
            debugPrint("LD A, (HL+)");
            // printf("arg: 0x%x\n", RegAF.hi);
        }
        break;
        case 0x2b: {
            // DEC HL
            RegHL.reg--;
            debugPrint("DEC HL");
        }
        break;
        case 0x2c: {
            // INC L
            // Flags: Z0H-
            inc8(RegHL.hi);
            debugPrint("INC L");
        }
        break;
        case 0x2d: {
            // DEC L
            // Flags: Z1H-
            dec8(RegHL.lo);
            debugPrint("DEC L");
        }
        break;
        case 0x2e: {
            // LD L, u8
            RegHL.lo = memory->readByte(programCounter);
            programCounter++;
            debugPrint("LD L, u8");
        }
        break;
        case 0x2f: {
            // CPL 
            // Flags: -11-
            RegAF.hi = ~RegAF.hi;
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, 1);
            debugPrint("CPL ");
        }
        break;
        case 0x30: {
            // JR NC, i8
            time += 8;		int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(!getFlag(FLAG_C)){
                programCounter += jumpBy;
                time += 4;
            }
            debugPrint("JR NC, i8");
        }
        break;
        case 0x31: {
            // LD SP, u16
            StackPointer.reg = memory->readWord(programCounter);
            programCounter += 2;
            debugPrint("LD SP, u16");
        }
        break;
        case 0x32: {
            // LD (HL-), A
            memory->writeByte(RegHL.reg--, RegAF.hi);
            debugPrint("LD (HL-), A");
        }
        break;
        case 0x33: {
            // INC SP
            StackPointer.reg++;
            debugPrint("INC SP");
        }
        break;
        case 0x34: {
            // INC (HL)
            // Flags: Z0H-
            uint8_t val = memory->readByte(RegHL.reg);
            inc8(val);
            memory->writeByte(RegHL.reg, val);
            debugPrint("INC (HL)");
        }
        break;
        case 0x35: {
            // DEC (HL)
            // Flags: Z1H-
            uint8_t val = memory->readByte(RegHL.reg);
            dec8(val);
            memory->writeByte(RegHL.reg, val);
            debugPrint("DEC (HL)");
        }
        break;
        case 0x36: {
            // LD (HL), u8
            memory->writeByte(RegHL.reg, memory->readByte(programCounter));
            programCounter++;
            debugPrint("LD (HL), u8");
        }
        break;
        case 0x37: {
            // SCF 
            // Flags: -001
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 1);
            debugPrint("SCF ");
        }
        break;
        case 0x38: {
            // JR C, i8
            time += 8;		
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(getFlag(FLAG_C)){
                programCounter += jumpBy;
                time += 4;
            }
            debugPrint("JR C, i8");
        }
        break;
        case 0x39: {
            // ADD HL, SP
            // Flags: -0HC
            add16(RegHL.reg, StackPointer.reg);
            RegHL.reg += StackPointer.reg;
            debugPrint("ADD HL, SP");
        }
        break;
        case 0x3a: {
            // LD A, (HL-)
            RegAF.hi = memory->readByte(RegHL.reg--);
            debugPrint("LD A, (HL-)");
        }
        break;
        case 0x3b: {
            // DEC SP
            StackPointer.reg--;
            debugPrint("DEC SP");
        }
        break;
        case 0x3c: {
            // INC A
            // Flags: Z0H-
            inc8(RegAF.hi);
            debugPrint("INC A");
        }
        break;
        case 0x3d: {
            // DEC A
            // Flags: Z1H-
            RegAF.hi--;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, 1));
            debugPrint("DEC A");
        }
        break;
        case 0x3e: {
            // LD A, u8
            RegAF.hi = memory->readByte(programCounter++);
            debugPrint("LD A, u8");
            // printf("arg: 0x%x\n", RegAF.hi);
        }
        break;
        case 0x3f: {
            // CCF 
            // Flags: -00C
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, !getFlag(FLAG_C));
            debugPrint("CCF ");
        }
        break;
        case 0x40: {
            // LD B, B
            RegBC.hi = RegBC.hi;
            debugPrint("LD B, B");
        }
        break;
        case 0x41: {
            // LD B, C
            RegBC.hi = RegBC.lo;
            debugPrint("LD B, C");
        }
        break;
        case 0x42: {
            // LD B, D
            RegBC.hi = RegDE.hi;
            debugPrint("LD B, D");
        }
        break;
        case 0x43: {
            // LD B, E
            RegBC.hi = RegDE.lo;
            debugPrint("LD B, E");
        }
        break;
        case 0x44: {
            // LD B, H
            RegBC.hi = RegHL.hi;
            debugPrint("LD B, H");
        }
        break;
        case 0x45: {
            // LD B, L
            RegBC.hi = RegHL.lo;
            debugPrint("LD B, L");
        }
        break;
        case 0x46: {
            // LD B, (HL)
            RegBC.hi = memory->readByte(RegHL.reg);
            debugPrint("LD B, (HL)");
        }
        break;
        case 0x47: {
            // LD B, A
            RegBC.hi = RegAF.hi;
            debugPrint("LD B, A");
        }
        break;
        case 0x48: {
            // LD C, B
            RegBC.lo = RegBC.hi;
            debugPrint("LD C, B");
        }
        break;
        case 0x49: {
            // LD C, C
            RegBC.lo = RegBC.lo;
            debugPrint("LD C, C");
        }
        break;
        case 0x4a: {
            // LD C, D
            RegBC.lo = RegDE.hi;
            debugPrint("LD C, D");
        }
        break;
        case 0x4b: {
            // LD C, E
            RegBC.lo = RegDE.lo;
            debugPrint("LD C, E");
        }
        break;
        case 0x4c: {
            // LD C, H
            RegBC.lo = RegHL.hi;
            debugPrint("LD C, H");
        }
        break;
        case 0x4d: {
            // LD C, L
            RegBC.lo = RegHL.lo;
            debugPrint("LD C, L");
        }
        break;
        case 0x4e: {
            // LD C, (HL)
            RegBC.lo = memory->readByte(RegHL.reg);
            debugPrint("LD C, (HL)");
        }
        break;
        case 0x4f: {
            // LD C, A
            RegBC.lo = RegAF.hi;
            debugPrint("LD C, A");
        }
        break;
        case 0x50: {
            // LD D, B
            RegDE.hi = RegBC.hi;
            debugPrint("LD D, B");
        }
        break;
        case 0x51: {
            // LD D, C
            RegDE.hi = RegBC.lo;
            debugPrint("LD D, C");
        }
        break;
        case 0x52: {
            // LD D, D
            RegDE.hi = RegDE.hi;
            debugPrint("LD D, D");
        }
        break;
        case 0x53: {
            // LD D, E
            RegDE.hi = RegDE.lo;
            debugPrint("LD D, E");
        }
        break;
        case 0x54: {
            // LD D, H
            RegDE.hi = RegHL.hi;
            debugPrint("LD D, H");
        }
        break;
        case 0x55: {
            // LD D, L
            RegDE.hi = RegHL.lo;
            debugPrint("LD D, L");
        }
        break;
        case 0x56: {
            // LD D, (HL)
            RegDE.hi = memory->readByte(RegHL.reg);
            debugPrint("LD D, (HL)");
        }
        break;
        case 0x57: {
            // LD D, A
            RegDE.hi = RegAF.hi;
            debugPrint("LD D, A");
        }
        break;
        case 0x58: {
            // LD E, B
            RegDE.lo = RegBC.hi;
            debugPrint("LD E, B");
        }
        break;
        case 0x59: {
            // LD E, C
            RegDE.lo = RegBC.lo;
            debugPrint("LD E, C");
        }
        break;
        case 0x5a: {
            // LD E, D
            RegDE.lo = RegDE.hi;
            debugPrint("LD E, D");
        }
        break;
        case 0x5b: {
            // LD E, E
            RegDE.lo = RegDE.lo;
            debugPrint("LD E, E");
        }
        break;
        case 0x5c: {
            // LD E, H
            RegDE.lo = RegHL.hi;
            debugPrint("LD E, H");
        }
        break;
        case 0x5d: {
            // LD E, L
            RegDE.lo = RegHL.lo;
            debugPrint("LD E, L");
        }
        break;
        case 0x5e: {
            // LD E, (HL)
            RegDE.lo = memory->readByte(RegHL.reg);
            debugPrint("LD E, (HL)");
        }
        break;
        case 0x5f: {
            // LD E, A
            RegDE.lo = RegAF.hi;
            debugPrint("LD E, A");
        }
        break;
        case 0x60: {
            // LD H, B
            RegHL.hi = RegBC.hi;
            debugPrint("LD H, B");
        }
        break;
        case 0x61: {
            // LD H, C
            RegHL.hi = RegBC.lo;
            debugPrint("LD H, C");
        }
        break;
        case 0x62: {
            // LD H, D
            RegHL.hi = RegDE.hi;
            debugPrint("LD H, D");
        }
        break;
        case 0x63: {
            // LD H, E
            RegHL.hi = RegDE.lo;
            debugPrint("LD H, E");
        }
        break;
        case 0x64: {
            // LD H, H
            RegHL.hi = RegHL.hi;
            debugPrint("LD H, H");
        }
        break;
        case 0x65: {
            // LD H, L
            RegHL.hi = RegHL.lo;
            debugPrint("LD H, L");
        }
        break;
        case 0x66: {
            // LD H, (HL)
            RegHL.hi = memory->readByte(RegHL.reg);
            debugPrint("LD H, (HL)");
        }
        break;
        case 0x67: {
            // LD H, A
            RegHL.hi = RegAF.hi;
            debugPrint("LD H, A");
        }
        break;
        case 0x68: {
            // LD L, B
            RegHL.lo = RegBC.hi;
            debugPrint("LD L, B");
        }
        break;
        case 0x69: {
            // LD L, C
            RegHL.lo = RegBC.lo;
            debugPrint("LD L, C");
        }
        break;
        case 0x6a: {
            // LD L, D
            RegHL.lo = RegDE.hi;
            debugPrint("LD L, D");
        }
        break;
        case 0x6b: {
            // LD L, E
            RegHL.lo = RegDE.lo;
            debugPrint("LD L, E");
        }
        break;
        case 0x6c: {
            // LD L, H
            RegHL.lo = RegHL.hi;
            debugPrint("LD L, H");
        }
        break;
        case 0x6d: {
            // LD L, L
            RegHL.lo = RegHL.lo;
            debugPrint("LD L, L");
        }
        break;
        case 0x6e: {
            // LD L, (HL)
            RegHL.lo = memory->readByte(RegHL.reg);
            debugPrint("LD L, (HL)");
        }
        break;
        case 0x6f: {
            // LD L, A
            RegHL.lo = RegAF.hi;
            debugPrint("LD L, A");
        }
        break;
        case 0x70: {
            // LD (HL), B
            memory->writeByte(RegHL.reg, RegBC.hi);
            debugPrint("LD (HL), B");
        }
        break;
        case 0x71: {
            // LD (HL), C
            memory->writeByte(RegHL.reg, RegBC.lo);
            debugPrint("LD (HL), C");
        }
        break;
        case 0x72: {
            // LD (HL), D
            memory->writeByte(RegHL.reg, RegDE.hi);
            debugPrint("LD (HL), D");
        }
        break;
        case 0x73: {
            // LD (HL), E
            memory->writeByte(RegHL.reg, RegDE.lo);
            debugPrint("LD (HL), E");
        }
        break;
        case 0x74: {
            // LD (HL), H
            memory->writeByte(RegHL.reg, RegHL.hi);
            debugPrint("LD (HL), H");
        }
        break;
        case 0x75: {
            // LD (HL), L
            memory->writeByte(RegHL.reg, RegHL.lo);
            debugPrint("LD (HL), L");
        }
        break;
        case 0x76: {
            // HALT 
            debugPrint("HALT ");
        }
        break;
        case 0x77: {
            // LD (HL), A
            memory->writeByte(RegHL.reg, RegAF.hi);
            debugPrint("LD (HL), A");
        }
        break;
        case 0x78: {
            // LD A, B
            RegAF.hi = RegBC.hi;
            debugPrint("LD A, B");
        }
        break;
        case 0x79: {
            // LD A, C
            RegAF.hi = RegBC.lo;
            debugPrint("LD A, C");
        }
        break;
        case 0x7a: {
            // LD A, D
            RegAF.hi = RegDE.hi;
            debugPrint("LD A, D");
        }
        break;
        case 0x7b: {
            // LD A, E
            RegAF.hi = RegDE.lo;
            debugPrint("LD A, E");
        }
        break;
        case 0x7c: {
            // LD A, H
            RegAF.hi = RegHL.hi;
            debugPrint("LD A, H");
        }
        break;
        case 0x7d: {
            // LD A, L
            RegAF.hi = RegHL.lo;
            debugPrint("LD A, L");
        }
        break;
        case 0x7e: {
            // LD A, (HL)
            RegAF.hi = memory->readByte(RegHL.reg);
            debugPrint("LD A, (HL)");
        }
        break;
        case 0x7f: {
            // LD A, A
            RegAF.hi = RegAF.hi;
            debugPrint("LD A, A");
        }
        break;
        case 0x80: {
            // ADD A, B
            // Flags: Z0HC
            add8(RegAF.hi, RegBC.hi);
            debugPrint("ADD A, B");
        }
        break;
        case 0x81: {
            // ADD A, C
            // Flags: Z0HC
            add8(RegAF.hi, RegBC.lo);
            debugPrint("ADD A, C");
        }
        break;
        case 0x82: {
            // ADD A, D
            // Flags: Z0HC
            add8(RegAF.hi, RegDE.hi);
            debugPrint("ADD A, D");
        }
        break;
        case 0x83: {
            // ADD A, E
            // Flags: Z0HC
            add8(RegAF.hi, RegDE.lo);
            debugPrint("ADD A, E");
        }
        break;
        case 0x84: {
            // ADD A, H
            // Flags: Z0HC
            add8(RegAF.hi, RegHL.hi);
            debugPrint("ADD A, H");
        }
        break;
        case 0x85: {
            // ADD A, L
            // Flags: Z0HC
            add8(RegAF.hi, RegHL.lo);
            debugPrint("ADD A, L");
        }
        break;
        case 0x86: {
            // ADD A, (HL)
            // Flags: Z0HC
            add8(RegAF.hi, memory->readByte(RegHL.reg));
            debugPrint("ADD A, (HL)");
        }
        break;
        case 0x87: {
            // ADD A, A
            // Flags: Z0HC
            add8(RegAF.hi, RegAF.hi);
            debugPrint("ADD A, A");
        }
        break;
        case 0x88: {
            // ADC A, B
            // Flags: Z0HC
            adc(RegAF.hi, RegBC.hi);
            debugPrint("ADC A, B");
        }
        break;
        case 0x89: {
            // ADC A, C
            // Flags: Z0HC
            adc(RegAF.hi, RegBC.lo);
            debugPrint("ADC A, C");
        }
        break;
        case 0x8a: {
            // ADC A, D
            // Flags: Z0HC
            adc(RegAF.hi, RegDE.hi);
            debugPrint("ADC A, D");
        }
        break;
        case 0x8b: {
            // ADC A, E
            // Flags: Z0HC
            adc(RegAF.hi, RegDE.lo);
            debugPrint("ADC A, E");
        }
        break;
        case 0x8c: {
            // ADC A, H
            // Flags: Z0HC
            adc(RegAF.hi, RegHL.hi);
            debugPrint("ADC A, H");
        }
        break;
        case 0x8d: {
            // ADC A, L
            // Flags: Z0HC
            adc(RegAF.hi, RegHL.lo);
            debugPrint("ADC A, L");
        }
        break;
        case 0x8e: {
            // ADC A, (HL)
            // Flags: Z0HC
            adc(RegAF.hi, memory->readByte(RegHL.reg));
            debugPrint("ADC A, (HL)");
        }
        break;
        case 0x8f: {
            // ADC A, A
            // Flags: Z0HC
            adc(RegAF.hi, RegAF.hi);
            debugPrint("ADC A, A");
        }
        break;
        case 0x90: {
            // SUB A, B
            // Flags: Z1HC
            sub(RegAF.hi, RegBC.hi);
            debugPrint("SUB A, B");
        }
        break;
        case 0x91: {
            // SUB A, C
            // Flags: Z1HC
            sub(RegAF.hi, RegBC.lo);
            debugPrint("SUB A, C");
        }
        break;
        case 0x92: {
            // SUB A, D
            // Flags: Z1HC
            sub(RegAF.hi, RegDE.hi);
            debugPrint("SUB A, D");
        }
        break;
        case 0x93: {
            // SUB A, E
            // Flags: Z1HC
            sub(RegAF.hi, RegDE.lo);
            debugPrint("SUB A, E");
        }
        break;
        case 0x94: {
            // SUB A, H
            // Flags: Z1HC
            sub(RegAF.hi, RegHL.hi);
            debugPrint("SUB A, H");
        }
        break;
        case 0x95: {
            // SUB A, L
            // Flags: Z1HC
            sub(RegAF.hi, RegHL.lo);
            debugPrint("SUB A, L");
        }
        break;
        case 0x96: {
            // SUB A, (HL)
            // Flags: Z1HC
            sub(RegAF.hi, memory->readByte(RegHL.reg));
            debugPrint("SUB A, (HL)");
        }
        break;
        case 0x97: {
            // SUB A, A
            // Flags: Z1HC
            sub(RegAF.hi, RegAF.hi);
            debugPrint("SUB A, A");
        }
        break;
        case 0x98: {
            // SBC A, B
            // Flags: Z1HC
            sbc(RegAF.hi, RegBC.hi);
            debugPrint("SBC A, B");
        }
        break;
        case 0x99: {
            // SBC A, C
            // Flags: Z1HC
            sbc(RegAF.hi, RegBC.lo);
            debugPrint("SBC A, C");
        }
        break;
        case 0x9a: {
            // SBC A, D
            // Flags: Z1HC
            sbc(RegAF.hi, RegDE.hi);
            debugPrint("SBC A, D");
        }
        break;
        case 0x9b: {
            // SBC A, E
            // Flags: Z1HC
            sbc(RegAF.hi, RegDE.lo);
            debugPrint("SBC A, E");
        }
        break;
        case 0x9c: {
            // SBC A, H
            // Flags: Z1HC
            sbc(RegAF.hi, RegHL.hi);
            debugPrint("SBC A, H");
        }
        break;
        case 0x9d: {
            // SBC A, L
            // Flags: Z1HC
            sbc(RegAF.hi, RegHL.lo);
            debugPrint("SBC A, L");
        }
        break;
        case 0x9e: {
            // SBC A, (HL)
            // Flags: Z1HC
            sbc(RegAF.hi, memory->readByte(RegHL.reg));
            debugPrint("SBC A, (HL)");
        }
        break;
        case 0x9f: {
            // SBC A, A
            // Flags: Z1HC
            sbc(RegAF.hi, RegAF.hi);
            debugPrint("SBC A, A");
        }
        break;
        case 0xa0: {
            // AND A, B
            // Flags: Z010
            RegAF.hi &= RegBC.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, B");
        }
        break;
        case 0xa1: {
            // AND A, C
            // Flags: Z010
            RegAF.hi &= RegBC.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, C");
        }
        break;
        case 0xa2: {
            // AND A, D
            // Flags: Z010
            RegAF.hi &= RegDE.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, D");
        }
        break;
        case 0xa3: {
            // AND A, E
            // Flags: Z010
            RegAF.hi &= RegDE.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, E");
        }
        break;
        case 0xa4: {
            // AND A, H
            // Flags: Z010
            RegAF.hi &= RegHL.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, H");
        }
        break;
        case 0xa5: {
            // AND A, L
            // Flags: Z010
            RegAF.hi &= RegHL.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, L");
        }
        break;
        case 0xa6: {
            // AND A, (HL)
            // Flags: Z010
            RegAF.hi &= memory->readByte(RegHL.reg);
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, (HL)");
        }
        break;
        case 0xa7: {
            // AND A, A
            // Flags: Z010
            RegAF.hi &= RegAF.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, A");
        }
        break;
        case 0xa8: {
            // XOR A, B
            // Flags: Z000
            RegAF.hi ^= RegBC.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, B");
        }
        break;
        case 0xa9: {
            // XOR A, C
            // Flags: Z000
            RegAF.hi ^= RegBC.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, C");
        }
        break;
        case 0xaa: {
            // XOR A, D
            // Flags: Z000
            RegAF.hi ^= RegDE.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, D");
        }
        break;
        case 0xab: {
            // XOR A, E
            // Flags: Z000
            RegAF.hi ^= RegDE.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, E");
        }
        break;
        case 0xac: {
            // XOR A, H
            // Flags: Z000
            RegAF.hi ^= RegHL.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, H");
        }
        break;
        case 0xad: {
            // XOR A, L
            // Flags: Z000
            RegAF.hi ^= RegHL.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, L");
        }
        break;
        case 0xae: {
            // XOR A, (HL)
            // Flags: Z000
            RegAF.hi ^= memory->readByte(RegHL.reg);
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, (HL)");
        }
        break;
        case 0xaf: {
            // XOR A, A
            // Flags: Z000
            RegAF.hi ^= RegAF.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, A");
        }
        break;
        case 0xb0: {
            // OR A, B
            // Flags: Z000
            RegAF.hi |= RegBC.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, B");
        }
        break;
        case 0xb1: {
            // OR A, C
            // Flags: Z000
            RegAF.hi |= RegBC.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, C");
        }
        break;
        case 0xb2: {
            // OR A, D
            // Flags: Z000
            RegAF.hi |= RegDE.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, D");
        }
        break;
        case 0xb3: {
            // OR A, E
            // Flags: Z000
            RegAF.hi |= RegDE.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, E");
        }
        break;
        case 0xb4: {
            // OR A, H
            // Flags: Z000
            RegAF.hi |= RegHL.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, H");
        }
        break;
        case 0xb5: {
            // OR A, L
            // Flags: Z000
            RegAF.hi |= RegHL.lo;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, L");
        }
        break;
        case 0xb6: {
            // OR A, (HL)
            // Flags: Z000
            RegAF.hi |= memory->readByte(RegHL.reg);
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, (HL)");
        }
        break;
        case 0xb7: {
            // OR A, A
            // Flags: Z000
            RegAF.hi |= RegAF.hi;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, A");
        }
        break;
        case 0xb8: {
            // CP A, B
            // Flags: Z1HC
            cp(RegAF.hi, RegBC.hi);
            debugPrint("CP A, B");
        }
        break;
        case 0xb9: {
            // CP A, C
            // Flags: Z1HC
            cp(RegAF.hi, RegBC.lo);
            debugPrint("CP A, C");
        }
        break;
        case 0xba: {
            // CP A, D
            // Flags: Z1HC
            cp(RegAF.hi, RegDE.hi);
            debugPrint("CP A, D");
        }
        break;
        case 0xbb: {
            // CP A, E
            // Flags: Z1HC
            cp(RegAF.hi, RegDE.lo);
            debugPrint("CP A, E");
        }
        break;
        case 0xbc: {
            // CP A, H
            // Flags: Z1HC
            cp(RegAF.hi, RegHL.hi);
            debugPrint("CP A, H");
        }
        break;
        case 0xbd: {
            // CP A, L
            // Flags: Z1HC
            cp(RegAF.hi, RegHL.lo);
            debugPrint("CP A, L");
        }
        break;
        case 0xbe: {
            // CP A, (HL)
            // Flags: Z1HC
            cp(RegAF.hi, memory->readByte(RegHL.reg));
            debugPrint("CP A, (HL)");
        }
        break;
        case 0xbf: {
            // CP A, A
            // Flags: Z1HC
            cp(RegAF.hi, RegAF.hi);
            debugPrint("CP A, A");
        }
        break;
        case 0xc0: {
            // RET NZ
            time += 8;
            if(!getFlag(FLAG_Z)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
                time += 12;
            }
            debugPrint("RET NZ");
        }
        break;
        case 0xc1: {
            // POP BC
            RegBC.reg = memory->readWord(StackPointer.reg);
            StackPointer.reg += 2;
            debugPrint("POP BC");
        }
        break;
        case 0xc2: {
            // JP NZ, u16
            time += 12;
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_Z)){
                programCounter = newAddress;
                time += 4;
            }
            debugPrint("JP NZ, u16");
        }
        break;
            case 0xc3: {
                // JP u16
                programCounter = memory->readWord(programCounter);
                debugPrint("JP u16");
                // printf("arg: 0x%x\n", programCounter);
            }
        break;
        case 0xc4: {
            // CALL NZ, u16
            time += 12;
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_Z)){
                StackPointer.reg -= 2;
                memory->writeWord(StackPointer.reg, programCounter);
                programCounter = newAddress;
                time += 12;
            }
            debugPrint("CALL NZ, u16");
        }
        break;
        case 0xc5: {
            // PUSH BC
            StackPointer.reg-= 2;
            memory->writeWord(StackPointer.reg, RegBC.reg);
            debugPrint("PUSH BC");
        }
        break;
        case 0xc6: {
            // ADD A, u8
            // Flags: Z0HC
            add8(RegAF.hi, memory->readByte(programCounter++));
            debugPrint("ADD A, u8");
        }
        break;
        case 0xc7: {
            // RST 00h
            uint8_t val = memory->readByte(0xc7);
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = 0;
            programCounter |= val;
            debugPrint("RST 00h");
        }
        break;
        case 0xc8: {
            // RET Z
            time += 8;
            if(getFlag(FLAG_Z)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
                time += 12;
            }
            debugPrint("RET Z");
        }
        break;
        case 0xc9: {
            // RET 
            programCounter = memory->readWord(StackPointer.reg);
            StackPointer.reg += 2;
            debugPrint("RET ");
        }
        break;
        case 0xca: {
            // JP Z, u16
            time += 12;
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_Z)){
                programCounter = newAddress;
                time += 4;
            }
            debugPrint("JP Z, u16");
        }
        break;
        case 0xcb: {
            // PREFIX CB
            time += executePrefixOP(memory->readByte(programCounter++));
            debugPrint("PREFIX CB");
        }
        break;
        case 0xcc: {
            // CALL Z, u16
            time += 12;
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_Z)){
                StackPointer.reg -= 2;
                memory->writeWord(StackPointer.reg, programCounter);
                programCounter = newAddress;
                time += 12;
            }
            debugPrint("CALL Z, u16");
        }
        break;
        case 0xcd: {
            // CALL u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = newAddress;
            debugPrint("CALL u16");
            // printf("arg: %x\n", newAddress);
        }
        break;
        case 0xce: {
            // ADC A, u8
            // Flags: Z0HC
            adc(RegAF.hi, memory->readByte(programCounter++));
            debugPrint("ADC A, u8");
        }
        break;
        case 0xcf: {
            // RST 08h
            uint8_t val = memory->readByte(0xcf);
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = 0;
            programCounter |= val;
            debugPrint("RST 08h");
        }
        break;
        case 0xd0: {
            // RET NC
            time += 8;
            if(!getFlag(FLAG_C)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
                time += 12;
            }
            debugPrint("RET NC");
        }
        break;
        case 0xd1: {
            // POP DE
            RegDE.reg = memory->readWord(StackPointer.reg);
            StackPointer.reg += 2;
            debugPrint("POP DE");
        }
        break;
        case 0xd2: {
            // JP NC, u16
            time += 12;
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_C)){
                programCounter = newAddress;
                time += 4;
            }
            debugPrint("JP NC, u16");
        }
        break;
        case 0xd4: {
            // CALL NC, u16
            time += 12;
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_C)){
                StackPointer.reg -= 2;
                memory->writeWord(StackPointer.reg, programCounter);
                programCounter = newAddress;
                time += 12;
            }
            debugPrint("CALL NC, u16");
        }
        break;
        case 0xd5: {
            // PUSH DE
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, RegDE.reg);
            debugPrint("PUSH DE");
        }
        break;
        case 0xd6: {
            // SUB A, u8
            // Flags: Z1HC
            sub(RegAF.hi, memory->readByte(programCounter++));
            debugPrint("SUB A, u8");
        }
        break;
        case 0xd7: {
            // RST 10h
            uint8_t val = memory->readByte(0xd7);
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = 0;
            programCounter |= val;
            debugPrint("RST 10h");
        }
        break;
        case 0xd8: {
            // RET C
            time += 8;
            if(getFlag(FLAG_C)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
                time += 12;
            }
            debugPrint("RET C");
        }
        break;
        case 0xd9: {
            // RETI 
            programCounter = memory->readWord(StackPointer.reg);
            StackPointer.reg += 2;
            interrupt->toggleIME(true);
            debugPrint("RETI ");
        }
        break;
        case 0xda: {
            // JP C, u16
            time += 12;
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_C)){
                programCounter = newAddress;
                time += 4;
            }
            debugPrint("JP C, u16");
        }
        break;
        case 0xdc: {
            // CALL C, u16
            time += 12;
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_C)){
                StackPointer.reg -= 2;
                memory->writeWord(StackPointer.reg, programCounter);
                programCounter = newAddress;
                time += 12;
            }
            debugPrint("CALL C, u16");
        }
        break;
        case 0xde: {
            // SBC A, u8
            // Flags: Z1HC
            uint8_t data = memory->readByte(programCounter++);
            sbc(RegAF.hi, data);
            debugPrint("SBC A, u8");
        }
        break;
        case 0xdf: {
            // RST 18h
            uint8_t val = memory->readByte(0xdf);
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = 0;
            programCounter |= val;
            debugPrint("RST 18h");
        }
        break;
        case 0xe0: {
            // LD (FF00+u8), A
            uint16_t addr = 0xFF00 + memory->readByte(programCounter++);
            memory->writeByte(addr, RegAF.hi);
            debugPrint("LD (FF00+u8), A");
            // printf("arg: 0x%x\n", addr);
        }
        break;
        case 0xe1: {
            // POP HL
            RegHL.reg = memory->readWord(StackPointer.reg);
            StackPointer.reg += 2;
            debugPrint("POP HL");
        }
        break;
        case 0xe2: {
            // LD (FF00+C), A
            memory->writeByte(0xFF00 + RegBC.lo, RegAF.hi);
            debugPrint("LD (FF00+C), A");
        }
        break;
        case 0xe5: {
            // PUSH HL
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, RegHL.reg);
            debugPrint("PUSH HL");
        }
        break;
        case 0xe6: {
            // AND A, u8
            // Flags: Z010
            uint8_t data = memory->readByte(programCounter++);
            RegAF.hi &= data;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            debugPrint("AND A, u8");
        }
        break;
        case 0xe7: {
            // RST 20h
            uint8_t val = memory->readByte(0xe7);
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = 0;
            programCounter |= val;
            debugPrint("RST 20h");
        }
        break;
        case 0xe8: {
            // ADD SP, i8
            // Flags: 00HC
            
            add16Signed(StackPointer.reg, (int) memory->readByte(programCounter++));
            debugPrint("ADD SP, i8");
        }
        break;
        case 0xe9: {
            // JP HL
            programCounter = RegHL.reg;
            debugPrint("JP HL");
        }
        break;
        case 0xea: {
            // LD (u16), A
            uint16_t addr = memory->readWord(programCounter);
            programCounter += 2;
            memory->writeByte(addr, RegAF.hi);
            debugPrint("LD (u16), A");
            // printf("arg: 0x%x\n", addr);
        }
        break;
        case 0xee: {
            // XOR A, u8
            // Flags: Z000
            uint8_t data = memory->readByte(programCounter++);
            RegAF.hi ^= data;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("XOR A, u8");
        }
        break;
        case 0xef: {
            // RST 28h
            uint8_t val = memory->readByte(0xef);
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = 0;
            programCounter |= val;
            debugPrint("RST 28h");
        }
        break;
        case 0xf0: {
            // LD A, (FF00+u8)
            uint16_t addr = 0xFF00 + memory->readByte(programCounter++);
            RegAF.hi = memory->readByte(addr);
            debugPrint("LD A, (FF00+u8)");
            // printf("arg: 0x%x\n", addr);
        }
        break;
        case 0xf1: {
            // POP AF
            // Flags: ZNHC
            RegAF.reg = memory->readWord(StackPointer.reg);
            StackPointer.reg += 2;
            // lower nibble of F register must be reset
            RegAF.lo &= 0xF0;
            debugPrint("POP AF");
        }
        break;
        case 0xf2: {
            // LD A, (FF00+C)
            RegAF.hi = memory->readByte(0xFF00 + RegBC.lo);
            debugPrint("LD A, (FF00+C)");
        }
        break;
        case 0xf3: {
            // DI 
            interrupt->toggleIME(false);
            lastInstructionEI = false;
            debugPrint("DI ");
        }
        break;
        case 0xf5: {
            // PUSH AF
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, RegAF.reg);
            debugPrint("PUSH AF");
        }
        break;
        case 0xf6: {
            // OR A, u8
            // Flags: Z000
            uint8_t data = memory->readByte(programCounter++);
            RegAF.hi |= data;
            setFlag(FLAG_Z, !RegAF.hi);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            debugPrint("OR A, u8");
        }
        break;
        case 0xf7: {
            // RST 30h
            uint8_t val = memory->readByte(0xf7);
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = 0;
            programCounter |= val;
            debugPrint("RST 30h");
        }
        break;
        case 0xf8: {
            // LD HL, SP+i8
            // Flags: 00HC
            setFlag(FLAG_Z, 0);
            setFlag(FLAG_N, 0);

            int arg = (int8_t) memory->readByte(programCounter++);
            uint16_t res = StackPointer.reg + arg;
            
            if(arg < 0){
                setFlag(FLAG_C, ((StackPointer.reg & 0xFF) + (arg)) > 0xFF);
                setFlag(FLAG_H, ((StackPointer.reg & 0xF) + (arg)) > 0xF);
            }
            else{
                setFlag(FLAG_C, (res & 0xFF) <= (StackPointer.reg & 0xFF));
                setFlag(FLAG_H, (res & 0xF) <= (StackPointer.reg & 0xF));
            }

            // perform 8 bit arithmetic
            RegHL.reg = StackPointer.reg + arg;
            
            
            debugPrint("LD HL, SP+i8");
        }
        break;
        case 0xf9: {
            // LD SP, HL
            StackPointer.reg = RegHL.reg;
            debugPrint("LD SP, HL");
        }
        break;
        case 0xfa: {
            // LD A, (u16)
            RegAF.hi = memory->readByte(programCounter);
            programCounter += 2;
            debugPrint("LD A, (u16)");
        }
        break;
        case 0xfb: {
            // EI 
            lastInstructionEI = true;
            debugPrint("EI ");
        }
        break;
        case 0xfe: {
            // CP A, u8
            // Flags: Z1HC
            cp(RegAF.hi, memory->readByte(programCounter++));
            debugPrint("CP A, u8");
        }
        break;
        case 0xff: {
            // RST 38h
            uint8_t val = memory->readByte(0xff);
            StackPointer.reg -= 2;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = 0;
            programCounter |= val;
            debugPrint("RST 38h");
        }
        break;
        default:
            std::stringstream ss;
            ss << std::hex << opCode;
            std::string str = "executing op at programCounter val 0x" + ss.str();
            debugPrint(str);
            break;
    }
    return time;
}

uint8_t CPU::executePrefixOP(uint8_t opCode){
    uint8_t time = prefixTimings[opCode];
    switch(opCode){
        case 0x00: {
            // RLC B
            // Flags: Z00C
            rlc(RegBC.hi);
            debugPrint("RLC B");
        }
        break;
        case 0x01: {
            // RLC C
            // Flags: Z00C
            rlc(RegBC.lo);
            debugPrint("RLC C");
        }
        break;
        case 0x02: {
            // RLC D
            // Flags: Z00C
            rlc(RegDE.hi);
            debugPrint("RLC D");
        }
        break;
        case 0x03: {
            // RLC E
            // Flags: Z00C
            rlc(RegDE.lo);
            debugPrint("RLC E");
        }
        break;
        case 0x04: {
            // RLC H
            // Flags: Z00C
            rlc(RegHL.hi);
            debugPrint("RLC H");
        }
        break;
        case 0x05: {
            // RLC L
            // Flags: Z00C
            rlc(RegHL.lo);
            debugPrint("RLC L");
        }
        break;
        case 0x06: {
            // RLC (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            rlc(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RLC (HL)");
        }
        break;
        case 0x07: {
            // RLC A
            // Flags: Z00C
            rlc(RegAF.hi);
            debugPrint("RLC A");
        }
        break;
        case 0x08: {
            // RRC B
            // Flags: Z00C
            rrc(RegBC.hi);
            debugPrint("RRC B");
        }
        break;
        case 0x09: {
            // RRC C
            // Flags: Z00C
            rrc(RegBC.lo);
            debugPrint("RRC C");
        }
        break;
        case 0x0a: {
            // RRC D
            // Flags: Z00C
            rrc(RegDE.hi);
            debugPrint("RRC D");
        }
        break;
        case 0x0b: {
            // RRC E
            // Flags: Z00C
            rrc(RegDE.lo);
            debugPrint("RRC E");
        }
        break;
        case 0x0c: {
            // RRC H
            // Flags: Z00C
            rrc(RegHL.hi);
            debugPrint("RRC H");
        }
        break;
        case 0x0d: {
            // RRC L
            // Flags: Z00C
            rrc(RegHL.lo);
            debugPrint("RRC L");
        }
        break;
        case 0x0e: {
            // RRC (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            rrc(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RRC (HL)");
        }
        break;
        case 0x0f: {
            // RRC A
            // Flags: Z00C
            rrc(RegAF.hi);
            debugPrint("RRC A");
        }
        break;
        case 0x10: {
            // RL B
            // Flags: Z00C
            rl(RegBC.hi);
            debugPrint("RL B");
        }
        break;
        case 0x11: {
            // RL C
            // Flags: Z00C
            rl(RegBC.lo);
            debugPrint("RL C");
        }
        break;
        case 0x12: {
            // RL D
            // Flags: Z00C
            rl(RegDE.hi);
            debugPrint("RL D");
        }
        break;
        case 0x13: {
            // RL E
            // Flags: Z00C
            rl(RegDE.lo);
            debugPrint("RL E");
        }
        break;
        case 0x14: {
            // RL H
            // Flags: Z00C
            rl(RegHL.hi);
            debugPrint("RL H");
        }
        break;
        case 0x15: {
            // RL L
            // Flags: Z00C
            rl(RegHL.lo);
            debugPrint("RL L");
        }
        break;
        case 0x16: {
            // RL (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            rl(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RL (HL)");
        }
        break;
        case 0x17: {
            // RL A
            // Flags: Z00C
            rl(RegAF.hi);
            debugPrint("RL A");
        }
        break;
        case 0x18: {
            // RR B
            // Flags: Z00C
            rr(RegBC.hi);
            debugPrint("RR B");
        }
        break;
        case 0x19: {
            // RR C
            // Flags: Z00C
            rr(RegBC.lo);
            debugPrint("RR C");
        }
        break;
        case 0x1a: {
            // RR D
            // Flags: Z00C
            rr(RegDE.hi);
            debugPrint("RR D");
        }
        break;
        case 0x1b: {
            // RR E
            // Flags: Z00C
            rr(RegDE.lo);
            debugPrint("RR E");
        }
        break;
        case 0x1c: {
            // RR H
            // Flags: Z00C
            rr(RegHL.hi);
            debugPrint("RR H");
        }
        break;
        case 0x1d: {
            // RR L
            // Flags: Z00C
            rr(RegHL.lo);
            debugPrint("RR L");
        }
        break;
        case 0x1e: {
            // RR (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            rr(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RR (HL)");
        }
        break;
        case 0x1f: {
            // RR A
            // Flags: Z00C
            rr(RegAF.hi);
            debugPrint("RR A");
        }
        break;
        case 0x20: {
            // SLA B
            // Flags: Z00C
            sla(RegBC.hi);
            debugPrint("SLA B");
        }
        break;
        case 0x21: {
            // SLA C
            // Flags: Z00C
            sla(RegBC.lo);
            debugPrint("SLA C");
        }
        break;
        case 0x22: {
            // SLA D
            // Flags: Z00C
            sla(RegDE.hi);
            debugPrint("SLA D");
        }
        break;
        case 0x23: {
            // SLA E
            // Flags: Z00C
            sla(RegDE.lo);
            debugPrint("SLA E");
        }
        break;
        case 0x24: {
            // SLA H
            // Flags: Z00C
            sla(RegHL.hi);
            debugPrint("SLA H");
        }
        break;
        case 0x25: {
            // SLA L
            // Flags: Z00C
            sla(RegHL.lo);
            debugPrint("SLA L");
        }
        break;
        case 0x26: {
            // SLA (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            sla(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SLA (HL)");
        }
        break;
        case 0x27: {
            // SLA A
            // Flags: Z00C
            sla(RegAF.hi);
            debugPrint("SLA A");
        }
        break;
        case 0x28: {
            // SRA B
            // Flags: Z00C
            sra(RegBC.hi);
            debugPrint("SRA B");
        }
        break;
        case 0x29: {
            // SRA C
            // Flags: Z00C
            sra(RegBC.lo);
            debugPrint("SRA C");
        }
        break;
        case 0x2a: {
            // SRA D
            // Flags: Z00C
            sra(RegDE.hi);
            debugPrint("SRA D");
        }
        break;
        case 0x2b: {
            // SRA E
            // Flags: Z00C
            sra(RegDE.lo);
            debugPrint("SRA E");
        }
        break;
        case 0x2c: {
            // SRA H
            // Flags: Z00C
            sra(RegHL.hi);
            debugPrint("SRA H");
        }
        break;
        case 0x2d: {
            // SRA L
            // Flags: Z00C
            sra(RegHL.lo);
            debugPrint("SRA L");
        }
        break;
        case 0x2e: {
            // SRA (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            sra(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SRA (HL)");
        }
        break;
        case 0x2f: {
            // SRA A
            // Flags: Z00C
            sra(RegAF.hi);
            debugPrint("SRA A");
        }
        break;
        case 0x30: {
            // SWAP B
            // Flags: Z000
            swap(RegBC.hi);
            debugPrint("SWAP B");
        }
        break;
        case 0x31: {
            // SWAP C
            // Flags: Z000
            swap(RegBC.lo);
            debugPrint("SWAP C");
        }
        break;
        case 0x32: {
            // SWAP D
            // Flags: Z000
            swap(RegDE.hi);
            debugPrint("SWAP D");
        }
        break;
        case 0x33: {
            // SWAP E
            // Flags: Z000
            swap(RegDE.lo);
            debugPrint("SWAP E");
        }
        break;
        case 0x34: {
            // SWAP H
            // Flags: Z000
            swap(RegHL.hi);
            debugPrint("SWAP H");
        }
        break;
        case 0x35: {
            // SWAP L
            // Flags: Z000
            swap(RegHL.lo);
            debugPrint("SWAP L");
        }
        break;
        case 0x36: {
            // SWAP (HL)
            // Flags: Z000
            uint8_t hlVal = memory->readByte(RegHL.reg);
            swap(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SWAP (HL)");
        }
        break;
        case 0x37: {
            // SWAP A
            // Flags: Z000
            swap(RegAF.hi);
            debugPrint("SWAP A");
        }
        break;
        case 0x38: {
            // SRL B
            // Flags: Z00C
            srl(RegBC.hi);
            debugPrint("SRL B");
        }
        break;
        case 0x39: {
            // SRL C
            // Flags: Z00C
            srl(RegBC.lo);
            debugPrint("SRL C");
        }
        break;
        case 0x3a: {
            // SRL D
            // Flags: Z00C
            srl(RegDE.hi);
            debugPrint("SRL D");
        }
        break;
        case 0x3b: {
            // SRL E
            // Flags: Z00C
            srl(RegDE.lo);
            debugPrint("SRL E");
        }
        break;
        case 0x3c: {
            // SRL H
            // Flags: Z00C
            srl(RegHL.hi);
            debugPrint("SRL H");
        }
        break;
        case 0x3d: {
            // SRL L
            // Flags: Z00C
            srl(RegHL.lo);
            debugPrint("SRL L");
        }
        break;
        case 0x3e: {
            // SRL (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            srl(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SRL (HL)");
        }
        break;
        case 0x3f: {
            // SRL A
            // Flags: Z00C
            srl(RegAF.hi);
            debugPrint("SRL A");
        }
        break;
        case 0x40: {
            // BIT 0, B
            // Flags: Z01-
            bit(0, RegBC.hi);
            debugPrint("BIT 0, B");
        }
        break;
        case 0x41: {
            // BIT 0, C
            // Flags: Z01-
            bit(0, RegBC.lo);
            debugPrint("BIT 0, C");
        }
        break;
        case 0x42: {
            // BIT 0, D
            // Flags: Z01-
            bit(0, RegDE.hi);
            debugPrint("BIT 0, D");
        }
        break;
        case 0x43: {
            // BIT 0, E
            // Flags: Z01-
            bit(0, RegDE.lo);
            debugPrint("BIT 0, E");
        }
        break;
        case 0x44: {
            // BIT 0, H
            // Flags: Z01-
            bit(0, RegHL.hi);
            debugPrint("BIT 0, H");
        }
        break;
        case 0x45: {
            // BIT 0, L
            // Flags: Z01-
            bit(0, RegHL.lo);
            debugPrint("BIT 0, L");
        }
        break;
        case 0x46: {
            // BIT 0, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(0, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("BIT 0, (HL)");
        }
        break;
        case 0x47: {
            // BIT 0, A
            // Flags: Z01-
            bit(0, RegAF.hi);
            debugPrint("BIT 0, A");
        }
        break;
        case 0x48: {
            // BIT 1, B
            // Flags: Z01-
            bit(1, RegBC.hi);
            debugPrint("BIT 1, B");
        }
        break;
        case 0x49: {
            // BIT 1, C
            // Flags: Z01-
            bit(1, RegBC.lo);
            debugPrint("BIT 1, C");
        }
        break;
        case 0x4a: {
            // BIT 1, D
            // Flags: Z01-
            bit(1, RegDE.hi);
            debugPrint("BIT 1, D");
        }
        break;
        case 0x4b: {
            // BIT 1, E
            // Flags: Z01-
            bit(1, RegDE.lo);
            debugPrint("BIT 1, E");
        }
        break;
        case 0x4c: {
            // BIT 1, H
            // Flags: Z01-
            bit(1, RegHL.hi);
            debugPrint("BIT 1, H");
        }
        break;
        case 0x4d: {
            // BIT 1, L
            // Flags: Z01-
            bit(1, RegHL.lo);
            debugPrint("BIT 1, L");
        }
        break;
        case 0x4e: {
            // BIT 1, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(1, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("BIT 1, (HL)");
        }
        break;
        case 0x4f: {
            // BIT 1, A
            // Flags: Z01-
            bit(1, RegAF.hi);
            debugPrint("BIT 1, A");
        }
        break;
        case 0x50: {
            // BIT 2, B
            // Flags: Z01-
            bit(2, RegBC.hi);
            debugPrint("BIT 2, B");
        }
        break;
        case 0x51: {
            // BIT 2, C
            // Flags: Z01-
            bit(2, RegBC.lo);
            debugPrint("BIT 2, C");
        }
        break;
        case 0x52: {
            // BIT 2, D
            // Flags: Z01-
            bit(2, RegDE.hi);
            debugPrint("BIT 2, D");
        }
        break;
        case 0x53: {
            // BIT 2, E
            // Flags: Z01-
            bit(2, RegDE.lo);
            debugPrint("BIT 2, E");
        }
        break;
        case 0x54: {
            // BIT 2, H
            // Flags: Z01-
            bit(2, RegHL.hi);
            debugPrint("BIT 2, H");
        }
        break;
        case 0x55: {
            // BIT 2, L
            // Flags: Z01-
            bit(2, RegHL.lo);
            debugPrint("BIT 2, L");
        }
        break;
        case 0x56: {
            // BIT 2, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(2, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("BIT 2, (HL)");
        }
        break;
        case 0x57: {
            // BIT 2, A
            // Flags: Z01-
            bit(2, RegAF.hi);
            debugPrint("BIT 2, A");
        }
        break;
        case 0x58: {
            // BIT 3, B
            // Flags: Z01-
            bit(3, RegBC.hi);
            debugPrint("BIT 3, B");
        }
        break;
        case 0x59: {
            // BIT 3, C
            // Flags: Z01-
            bit(3, RegBC.lo);
            debugPrint("BIT 3, C");
        }
        break;
        case 0x5a: {
            // BIT 3, D
            // Flags: Z01-
            bit(3, RegDE.hi);
            debugPrint("BIT 3, D");
        }
        break;
        case 0x5b: {
            // BIT 3, E
            // Flags: Z01-
            bit(3, RegDE.lo);
            debugPrint("BIT 3, E");
        }
        break;
        case 0x5c: {
            // BIT 3, H
            // Flags: Z01-
            bit(3, RegHL.hi);
            debugPrint("BIT 3, H");
        }
        break;
        case 0x5d: {
            // BIT 3, L
            // Flags: Z01-
            bit(3, RegHL.lo);
            debugPrint("BIT 3, L");
        }
        break;
        case 0x5e: {
            // BIT 3, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(3, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("BIT 3, (HL)");
        }
        break;
        case 0x5f: {
            // BIT 3, A
            // Flags: Z01-
            bit(3, RegAF.hi);
            debugPrint("BIT 3, A");
        }
        break;
        case 0x60: {
            // BIT 4, B
            // Flags: Z01-
            bit(4, RegBC.hi);
            debugPrint("BIT 4, B");
        }
        break;
        case 0x61: {
            // BIT 4, C
            // Flags: Z01-
            bit(4, RegBC.lo);
            debugPrint("BIT 4, C");
        }
        break;
        case 0x62: {
            // BIT 4, D
            // Flags: Z01-
            bit(4, RegDE.hi);
            debugPrint("BIT 4, D");
        }
        break;
        case 0x63: {
            // BIT 4, E
            // Flags: Z01-
            bit(4, RegDE.lo);
            debugPrint("BIT 4, E");
        }
        break;
        case 0x64: {
            // BIT 4, H
            // Flags: Z01-
            bit(4, RegHL.hi);
            debugPrint("BIT 4, H");
        }
        break;
        case 0x65: {
            // BIT 4, L
            // Flags: Z01-
            bit(4, RegHL.lo);
            debugPrint("BIT 4, L");
        }
        break;
        case 0x66: {
            // BIT 4, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(4, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("BIT 4, (HL)");
        }
        break;
        case 0x67: {
            // BIT 4, A
            // Flags: Z01-
            bit(4, RegAF.hi);
            debugPrint("BIT 4, A");
        }
        break;
        case 0x68: {
            // BIT 5, B
            // Flags: Z01-
            bit(5, RegBC.hi);
            debugPrint("BIT 5, B");
        }
        break;
        case 0x69: {
            // BIT 5, C
            // Flags: Z01-
            bit(5, RegBC.lo);
            debugPrint("BIT 5, C");
        }
        break;
        case 0x6a: {
            // BIT 5, D
            // Flags: Z01-
            bit(5, RegDE.hi);
            debugPrint("BIT 5, D");
        }
        break;
        case 0x6b: {
            // BIT 5, E
            // Flags: Z01-
            bit(5, RegDE.lo);
            debugPrint("BIT 5, E");
        }
        break;
        case 0x6c: {
            // BIT 5, H
            // Flags: Z01-
            bit(5, RegHL.hi);
            debugPrint("BIT 5, H");
        }
        break;
        case 0x6d: {
            // BIT 5, L
            // Flags: Z01-
            bit(5, RegHL.lo);
            debugPrint("BIT 5, L");
        }
        break;
        case 0x6e: {
            // BIT 5, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(5, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("BIT 5, (HL)");
        }
        break;
        case 0x6f: {
            // BIT 5, A
            // Flags: Z01-
            bit(5, RegAF.hi);
            debugPrint("BIT 5, A");
        }
        break;
        case 0x70: {
            // BIT 6, B
            // Flags: Z01-
            bit(6, RegBC.hi);
            debugPrint("BIT 6, B");
        }
        break;
        case 0x71: {
            // BIT 6, C
            // Flags: Z01-
            bit(6, RegBC.lo);
            debugPrint("BIT 6, C");
        }
        break;
        case 0x72: {
            // BIT 6, D
            // Flags: Z01-
            bit(6, RegDE.hi);
            debugPrint("BIT 6, D");
        }
        break;
        case 0x73: {
            // BIT 6, E
            // Flags: Z01-
            bit(6, RegDE.lo);
            debugPrint("BIT 6, E");
        }
        break;
        case 0x74: {
            // BIT 6, H
            // Flags: Z01-
            bit(6, RegHL.hi);
            debugPrint("BIT 6, H");
        }
        break;
        case 0x75: {
            // BIT 6, L
            // Flags: Z01-
            bit(6, RegHL.lo);
            debugPrint("BIT 6, L");
        }
        break;
        case 0x76: {
            // BIT 6, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(6, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("BIT 6, (HL)");
        }
        break;
        case 0x77: {
            // BIT 6, A
            // Flags: Z01-
            bit(6, RegAF.hi);
            debugPrint("BIT 6, A");
        }
        break;
        case 0x78: {
            // BIT 7, B
            // Flags: Z01-
            bit(7, RegBC.hi);
            debugPrint("BIT 7, B");
        }
        break;
        case 0x79: {
            // BIT 7, C
            // Flags: Z01-
            bit(7, RegBC.lo);
            debugPrint("BIT 7, C");
        }
        break;
        case 0x7a: {
            // BIT 7, D
            // Flags: Z01-
            bit(7, RegDE.hi);
            debugPrint("BIT 7, D");
        }
        break;
        case 0x7b: {
            // BIT 7, E
            // Flags: Z01-
            bit(7, RegDE.lo);
            debugPrint("BIT 7, E");
        }
        break;
        case 0x7c: {
            // BIT 7, H
            // Flags: Z01-
            bit(7, RegHL.hi);
            debugPrint("BIT 7, H");
        }
        break;
        case 0x7d: {
            // BIT 7, L
            // Flags: Z01-
            bit(7, RegHL.lo);
            debugPrint("BIT 7, L");
        }
        break;
        case 0x7e: {
            // BIT 7, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(7, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("BIT 7, (HL)");
        }
        break;
        case 0x7f: {
            // BIT 7, A
            // Flags: Z01-
            bit(7, RegAF.hi);
            debugPrint("BIT 7, A");
        }
        break;
        case 0x80: {
            // RES 0, B
            res(0, RegBC.hi);
            debugPrint("RES 0, B");
        }
        break;
        case 0x81: {
            // RES 0, C
            res(0, RegBC.lo);
            debugPrint("RES 0, C");
        }
        break;
        case 0x82: {
            // RES 0, D
            res(0, RegDE.hi);
            debugPrint("RES 0, D");
        }
        break;
        case 0x83: {
            // RES 0, E
            res(0, RegDE.lo);
            debugPrint("RES 0, E");
        }
        break;
        case 0x84: {
            // RES 0, H
            res(0, RegHL.hi);
            debugPrint("RES 0, H");
        }
        break;
        case 0x85: {
            // RES 0, L
            res(0, RegHL.lo);
            debugPrint("RES 0, L");
        }
        break;
        case 0x86: {
            // RES 0, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(0, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RES 0, (HL)");
        }
        break;
        case 0x87: {
            // RES 0, A
            res(0, RegAF.hi);
            debugPrint("RES 0, A");
        }
        break;
        case 0x88: {
            // RES 1, B
            res(1, RegBC.hi);
            debugPrint("RES 1, B");
        }
        break;
        case 0x89: {
            // RES 1, C
            res(1, RegBC.lo);
            debugPrint("RES 1, C");
        }
        break;
        case 0x8a: {
            // RES 1, D
            res(1, RegDE.hi);
            debugPrint("RES 1, D");
        }
        break;
        case 0x8b: {
            // RES 1, E
            res(1, RegDE.lo);
            debugPrint("RES 1, E");
        }
        break;
        case 0x8c: {
            // RES 1, H
            res(1, RegHL.hi);
            debugPrint("RES 1, H");
        }
        break;
        case 0x8d: {
            // RES 1, L
            res(1, RegHL.lo);
            debugPrint("RES 1, L");
        }
        break;
        case 0x8e: {
            // RES 1, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(1, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RES 1, (HL)");
        }
        break;
        case 0x8f: {
            // RES 1, A
            res(1, RegAF.hi);
            debugPrint("RES 1, A");
        }
        break;
        case 0x90: {
            // RES 2, B
            res(2, RegBC.hi);
            debugPrint("RES 2, B");
        }
        break;
        case 0x91: {
            // RES 2, C
            res(2, RegBC.lo);
            debugPrint("RES 2, C");
        }
        break;
        case 0x92: {
            // RES 2, D
            res(2, RegDE.hi);
            debugPrint("RES 2, D");
        }
        break;
        case 0x93: {
            // RES 2, E
            res(2, RegDE.lo);
            debugPrint("RES 2, E");
        }
        break;
        case 0x94: {
            // RES 2, H
            res(2, RegHL.hi);
            debugPrint("RES 2, H");
        }
        break;
        case 0x95: {
            // RES 2, L
            res(2, RegHL.lo);
            debugPrint("RES 2, L");
        }
        break;
        case 0x96: {
            // RES 2, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(2, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RES 2, (HL)");
        }
        break;
        case 0x97: {
            // RES 2, A
            res(2, RegAF.hi);
            debugPrint("RES 2, A");
        }
        break;
        case 0x98: {
            // RES 3, B
            res(3, RegBC.hi);
            debugPrint("RES 3, B");
        }
        break;
        case 0x99: {
            // RES 3, C
            res(3, RegBC.lo);
            debugPrint("RES 3, C");
        }
        break;
        case 0x9a: {
            // RES 3, D
            res(3, RegDE.hi);
            debugPrint("RES 3, D");
        }
        break;
        case 0x9b: {
            // RES 3, E
            res(3, RegDE.lo);
            debugPrint("RES 3, E");
        }
        break;
        case 0x9c: {
            // RES 3, H
            res(3, RegHL.hi);
            debugPrint("RES 3, H");
        }
        break;
        case 0x9d: {
            // RES 3, L
            res(3, RegHL.lo);
            debugPrint("RES 3, L");
        }
        break;
        case 0x9e: {
            // RES 3, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(3, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RES 3, (HL)");
        }
        break;
        case 0x9f: {
            // RES 3, A
            res(3, RegAF.hi);
            debugPrint("RES 3, A");
        }
        break;
        case 0xa0: {
            // RES 4, B
            res(4, RegBC.hi);
            debugPrint("RES 4, B");
        }
        break;
        case 0xa1: {
            // RES 4, C
            res(4, RegBC.lo);
            debugPrint("RES 4, C");
        }
        break;
        case 0xa2: {
            // RES 4, D
            res(4, RegDE.hi);
            debugPrint("RES 4, D");
        }
        break;
        case 0xa3: {
            // RES 4, E
            res(4, RegDE.lo);
            debugPrint("RES 4, E");
        }
        break;
        case 0xa4: {
            // RES 4, H
            res(4, RegHL.hi);
            debugPrint("RES 4, H");
        }
        break;
        case 0xa5: {
            // RES 4, L
            res(4, RegHL.lo);
            debugPrint("RES 4, L");
        }
        break;
        case 0xa6: {
            // RES 4, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(4, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RES 4, (HL)");
        }
        break;
        case 0xa7: {
            // RES 4, A
            res(4, RegAF.hi);
            debugPrint("RES 4, A");
        }
        break;
        case 0xa8: {
            // RES 5, B
            res(5, RegBC.hi);
            debugPrint("RES 5, B");
        }
        break;
        case 0xa9: {
            // RES 5, C
            res(5, RegBC.lo);
            debugPrint("RES 5, C");
        }
        break;
        case 0xaa: {
            // RES 5, D
            res(5, RegDE.hi);
            debugPrint("RES 5, D");
        }
        break;
        case 0xab: {
            // RES 5, E
            res(5, RegDE.lo);
            debugPrint("RES 5, E");
        }
        break;
        case 0xac: {
            // RES 5, H
            res(5, RegHL.hi);
            debugPrint("RES 5, H");
        }
        break;
        case 0xad: {
            // RES 5, L
            res(5, RegHL.lo);
            debugPrint("RES 5, L");
        }
        break;
        case 0xae: {
            // RES 5, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(5, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RES 5, (HL)");
        }
        break;
        case 0xaf: {
            // RES 5, A
            res(5, RegAF.hi);
            debugPrint("RES 5, A");
        }
        break;
        case 0xb0: {
            // RES 6, B
            res(6, RegBC.hi);
            debugPrint("RES 6, B");
        }
        break;
        case 0xb1: {
            // RES 6, C
            res(6, RegBC.lo);
            debugPrint("RES 6, C");
        }
        break;
        case 0xb2: {
            // RES 6, D
            res(6, RegDE.hi);
            debugPrint("RES 6, D");
        }
        break;
        case 0xb3: {
            // RES 6, E
            res(6, RegDE.lo);
            debugPrint("RES 6, E");
        }
        break;
        case 0xb4: {
            // RES 6, H
            res(6, RegHL.hi);
            debugPrint("RES 6, H");
        }
        break;
        case 0xb5: {
            // RES 6, L
            res(6, RegHL.lo);
            debugPrint("RES 6, L");
        }
        break;
        case 0xb6: {
            // RES 6, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(6, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RES 6, (HL)");
        }
        break;
        case 0xb7: {
            // RES 6, A
            res(6, RegAF.hi);
            debugPrint("RES 6, A");
        }
        break;
        case 0xb8: {
            // RES 7, B
            res(7, RegBC.hi);
            debugPrint("RES 7, B");
        }
        break;
        case 0xb9: {
            // RES 7, C
            res(7, RegBC.lo);
            debugPrint("RES 7, C");
        }
        break;
        case 0xba: {
            // RES 7, D
            res(7, RegDE.hi);
            debugPrint("RES 7, D");
        }
        break;
        case 0xbb: {
            // RES 7, E
            res(7, RegDE.lo);
            debugPrint("RES 7, E");
        }
        break;
        case 0xbc: {
            // RES 7, H
            res(7, RegHL.hi);
            debugPrint("RES 7, H");
        }
        break;
        case 0xbd: {
            // RES 7, L
            res(7, RegHL.lo);
            debugPrint("RES 7, L");
        }
        break;
        case 0xbe: {
            // RES 7, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(7, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("RES 7, (HL)");
        }
        break;
        case 0xbf: {
            // RES 7, A
            res(7, RegAF.hi);
            debugPrint("RES 7, A");
        }
        break;
        case 0xc0: {
            // SET 0, B
            set(0, RegBC.hi);
            debugPrint("SET 0, B");
        }
        break;
        case 0xc1: {
            // SET 0, C
            set(0, RegBC.lo);
            debugPrint("SET 0, C");
        }
        break;
        case 0xc2: {
            // SET 0, D
            set(0, RegDE.hi);
            debugPrint("SET 0, D");
        }
        break;
        case 0xc3: {
            // SET 0, E
            set(0, RegDE.lo);
            debugPrint("SET 0, E");
        }
        break;
        case 0xc4: {
            // SET 0, H
            set(0, RegHL.hi);
            debugPrint("SET 0, H");
        }
        break;
        case 0xc5: {
            // SET 0, L
            set(0, RegHL.lo);
            debugPrint("SET 0, L");
        }
        break;
        case 0xc6: {
            // SET 0, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(0, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SET 0, (HL)");
        }
        break;
        case 0xc7: {
            // SET 0, A
            set(0, RegAF.hi);
            debugPrint("SET 0, A");
        }
        break;
        case 0xc8: {
            // SET 1, B
            set(1, RegBC.hi);
            debugPrint("SET 1, B");
        }
        break;
        case 0xc9: {
            // SET 1, C
            set(1, RegBC.lo);
            debugPrint("SET 1, C");
        }
        break;
        case 0xca: {
            // SET 1, D
            set(1, RegDE.hi);
            debugPrint("SET 1, D");
        }
        break;
        case 0xcb: {
            // SET 1, E
            set(1, RegDE.lo);
            debugPrint("SET 1, E");
        }
        break;
        case 0xcc: {
            // SET 1, H
            set(1, RegHL.hi);
            debugPrint("SET 1, H");
        }
        break;
        case 0xcd: {
            // SET 1, L
            set(1, RegHL.lo);
            debugPrint("SET 1, L");
        }
        break;
        case 0xce: {
            // SET 1, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(1, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SET 1, (HL)");
        }
        break;
        case 0xcf: {
            // SET 1, A
            set(1, RegAF.hi);
            debugPrint("SET 1, A");
        }
        break;
        case 0xd0: {
            // SET 2, B
            set(2, RegBC.hi);
            debugPrint("SET 2, B");
        }
        break;
        case 0xd1: {
            // SET 2, C
            set(2, RegBC.lo);
            debugPrint("SET 2, C");
        }
        break;
        case 0xd2: {
            // SET 2, D
            set(2, RegDE.hi);
            debugPrint("SET 2, D");
        }
        break;
        case 0xd3: {
            // SET 2, E
            set(2, RegDE.lo);
            debugPrint("SET 2, E");
        }
        break;
        case 0xd4: {
            // SET 2, H
            set(2, RegHL.hi);
            debugPrint("SET 2, H");
        }
        break;
        case 0xd5: {
            // SET 2, L
            set(2, RegHL.lo);
            debugPrint("SET 2, L");
        }
        break;
        case 0xd6: {
            // SET 2, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(2, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SET 2, (HL)");
        }
        break;
        case 0xd7: {
            // SET 2, A
            set(2, RegAF.hi);
            debugPrint("SET 2, A");
        }
        break;
        case 0xd8: {
            // SET 3, B
            set(3, RegBC.hi);
            debugPrint("SET 3, B");
        }
        break;
        case 0xd9: {
            // SET 3, C
            set(3, RegBC.lo);
            debugPrint("SET 3, C");
        }
        break;
        case 0xda: {
            // SET 3, D
            set(3, RegDE.hi);
            debugPrint("SET 3, D");
        }
        break;
        case 0xdb: {
            // SET 3, E
            set(3, RegDE.lo);
            debugPrint("SET 3, E");
        }
        break;
        case 0xdc: {
            // SET 3, H
            set(3, RegHL.hi);
            debugPrint("SET 3, H");
        }
        break;
        case 0xdd: {
            // SET 3, L
            set(3, RegHL.lo);
            debugPrint("SET 3, L");
        }
        break;
        case 0xde: {
            // SET 3, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(3, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SET 3, (HL)");
        }
        break;
        case 0xdf: {
            // SET 3, A
            set(3, RegAF.hi);
            debugPrint("SET 3, A");
        }
        break;
        case 0xe0: {
            // SET 4, B
            set(4, RegBC.hi);
            debugPrint("SET 4, B");
        }
        break;
        case 0xe1: {
            // SET 4, C
            set(4, RegBC.lo);
            debugPrint("SET 4, C");
        }
        break;
        case 0xe2: {
            // SET 4, D
            set(4, RegDE.hi);
            debugPrint("SET 4, D");
        }
        break;
        case 0xe3: {
            // SET 4, E
            set(4, RegDE.lo);
            debugPrint("SET 4, E");
        }
        break;
        case 0xe4: {
            // SET 4, H
            set(4, RegHL.hi);
            debugPrint("SET 4, H");
        }
        break;
        case 0xe5: {
            // SET 4, L
            set(4, RegHL.lo);
            debugPrint("SET 4, L");
        }
        break;
        case 0xe6: {
            // SET 4, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(4, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SET 4, (HL)");
        }
        break;
        case 0xe7: {
            // SET 4, A
            set(4, RegAF.hi);
            debugPrint("SET 4, A");
        }
        break;
        case 0xe8: {
            // SET 5, B
            set(5, RegBC.hi);
            debugPrint("SET 5, B");
        }
        break;
        case 0xe9: {
            // SET 5, C
            set(5, RegBC.lo);
            debugPrint("SET 5, C");
        }
        break;
        case 0xea: {
            // SET 5, D
            set(5, RegDE.hi);
            debugPrint("SET 5, D");
        }
        break;
        case 0xeb: {
            // SET 5, E
            set(5, RegDE.lo);
            debugPrint("SET 5, E");
        }
        break;
        case 0xec: {
            // SET 5, H
            set(5, RegHL.hi);
            debugPrint("SET 5, H");
        }
        break;
        case 0xed: {
            // SET 5, L
            set(5, RegHL.lo);
            debugPrint("SET 5, L");
        }
        break;
        case 0xee: {
            // SET 5, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(5, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SET 5, (HL)");
        }
        break;
        case 0xef: {
            // SET 5, A
            set(5, RegAF.hi);
            debugPrint("SET 5, A");
        }
        break;
        case 0xf0: {
            // SET 6, B
            set(6, RegBC.hi);
            debugPrint("SET 6, B");
        }
        break;
        case 0xf1: {
            // SET 6, C
            set(6, RegBC.lo);
            debugPrint("SET 6, C");
        }
        break;
        case 0xf2: {
            // SET 6, D
            set(6, RegDE.hi);
            debugPrint("SET 6, D");
        }
        break;
        case 0xf3: {
            // SET 6, E
            set(6, RegDE.lo);
            debugPrint("SET 6, E");
        }
        break;
        case 0xf4: {
            // SET 6, H
            set(6, RegHL.hi);
            debugPrint("SET 6, H");
        }
        break;
        case 0xf5: {
            // SET 6, L
            set(6, RegHL.lo);
            debugPrint("SET 6, L");
        }
        break;
        case 0xf6: {
            // SET 6, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(6, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SET 6, (HL)");
        }
        break;
        case 0xf7: {
            // SET 6, A
            set(6, RegAF.hi);
            debugPrint("SET 6, A");
        }
        break;
        case 0xf8: {
            // SET 7, B
            set(7, RegBC.hi);
            debugPrint("SET 7, B");
        }
        break;
        case 0xf9: {
            // SET 7, C
            set(7, RegBC.lo);
            debugPrint("SET 7, C");
        }
        break;
        case 0xfa: {
            // SET 7, D
            set(7, RegDE.hi);
            debugPrint("SET 7, D");
        }
        break;
        case 0xfb: {
            // SET 7, E
            set(7, RegDE.lo);
            debugPrint("SET 7, E");
        }
        break;
        case 0xfc: {
            // SET 7, H
            set(7, RegHL.hi);
            debugPrint("SET 7, H");
        }
        break;
        case 0xfd: {
            // SET 7, L
            set(7, RegHL.lo);
            debugPrint("SET 7, L");
        }
        break;
        case 0xfe: {
            // SET 7, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(7, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            debugPrint("SET 7, (HL)");
        }
        break;
        case 0xff: {
            // SET 7, A
            set(7, RegAF.hi);
            debugPrint("SET 7, A");
        }
        break;
        default:
            std::cout << "invalid or unimplemented extended op code" << std::hex << opCode << std::endl;
            break;
    }
    return time;
}