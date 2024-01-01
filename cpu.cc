#include <iostream>
#include <fstream>

#include "cpu.hh"

/**
 * @brief 
 * Emulate the internal state of the gameboy Z80
 */

CPU::CPU(Memory *memory, Interrupt *interrupt){
    programCounter = 0x100;
    RegAF.reg = 0x01B0;
    RegBC.reg = 0x0013;
    RegDE.reg = 0x00D8;
    RegHL.reg = 0x014D;

    StackPointer.reg = 0xFFFE;

    this->memory = memory;
    this->interrupt = interrupt;
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

u_int8_t CPU::halfCarry8(uint8_t a, uint8_t b){
    return (((a & 0xf) + (b & 0xf)) & 0x10) == 0x10;
}

u_int8_t CPU::halfCarry16(uint16_t a, uint16_t b){
    return (((a & 0xff) + (b & 0xff)) & 0x100) == 0x100;
}

void CPU::resetFlags(){
    RegAF.lo = 0;
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

void CPU::step(){
    executeOP(memory->readByte(programCounter++));
}

void CPU::executeOP(uint8_t opCode){
    switch(opCode){
        case 0x00: {
            // NOP 
            std::cout<<"NOP "<<std::endl;
        }
        break;
        case 0x01: {
            // LD BC, u16
            RegBC.reg = memory->readWord(programCounter);
            programCounter += 2;
            std::cout<<"LD BC, u16"<<std::endl;
        }
        break;
        case 0x02: {
            // LD (BC), A
            memory->writeByte(RegBC.reg, RegAF.hi);
            std::cout<<"LD (BC), A"<<std::endl;
        }
        break;
        case 0x03: {
            // INC BC
            RegBC.reg++;
            std::cout<<"INC BC"<<std::endl;
        }
        break;
        case 0x04: {
            // INC B
            // Flags: Z0H-
            RegBC.hi++;
            if(RegBC.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegBC.hi, 1));
            std::cout<<"INC B"<<std::endl;
        }
        break;
        case 0x05: {
            // DEC B
            // Flags: Z1H-
            RegBC.hi--;
            if(RegBC.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegBC.hi, 1));
            std::cout<<"DEC B"<<std::endl;
        }
        break;
        case 0x06: {
            // LD B, u8
            RegBC.hi = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD B, u8"<<std::endl;
        }
        break;
        case 0x07: {
            // RLCA 
            // Flags: 000C
            rlca();
            std::cout<<"RLCA "<<std::endl;
        }
        break;
        case 0x08: {
            // LD (u16), SP
            memory->writeWord(memory->readWord(programCounter), StackPointer.reg);
            programCounter += 2;
            std::cout<<"LD (u16), SP"<<std::endl;
        }
        break;
        case 0x09: {
            // ADD HL, BC
            // Flags: -0HC
            RegHL.reg += RegBC.reg;
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(RegHL.reg, RegBC.reg));
            setFlag(FLAG_C, RegHL.reg > 0xffff);
            std::cout<<"ADD HL, BC"<<std::endl;
        }
        break;
        case 0x0a: {
            // LD A, (BC)
            RegAF.hi = memory->readByte(RegBC.reg);
            std::cout<<"LD A, (BC)"<<std::endl;
        }
        break;
        case 0x0b: {
            // DEC BC
            RegBC.reg--;
            std::cout<<"DEC BC"<<std::endl;
        }
        break;
        case 0x0c: {
            // INC C
            // Flags: Z0H-
            RegBC.lo++;
            if(RegBC.lo == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegBC.lo, 1));
            std::cout<<"INC C"<<std::endl;
        }
        break;
        case 0x0d: {
            // DEC C
            // Flags: Z1H-
            RegBC.lo--;
            if(RegBC.lo == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegBC.lo, 1));
            std::cout<<"DEC C"<<std::endl;
        }
        break;
        case 0x0e: {
            // LD C, u8
            RegBC.lo = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD C, u8"<<std::endl;
        }
        break;
        case 0x0f: {
            // RRCA 
            // Flags: 000C
            rrca();
            std::cout<<"RRCA "<<std::endl;
        }
        break;
        case 0x10: {
            // STOP 
            std::cout<<"STOP "<<std::endl;
        }
        break;
        case 0x11: {
            // LD DE, u16
            RegDE.reg = memory->readWord(programCounter);
            programCounter += 2;
            std::cout<<"LD DE, u16"<<std::endl;
        }
        break;
        case 0x12: {
            // LD (DE), A
            memory->writeByte(RegDE.reg, RegAF.hi);
            std::cout<<"LD (DE), A"<<std::endl;
        }
        break;
        case 0x13: {
            // INC DE
            RegDE.reg++;
            std::cout<<"INC DE"<<std::endl;
        }
        break;
        case 0x14: {
            // INC D
            // Flags: Z0H-
            RegDE.hi++;
            if(RegDE.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegDE.hi, 1));
            std::cout<<"INC D"<<std::endl;
        }
        break;
        case 0x15: {
            // DEC D
            // Flags: Z1H-
            RegDE.hi--;
            if(RegDE.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegDE.hi, 1));
            std::cout<<"DEC D"<<std::endl;
        }
        break;
        case 0x16: {
            // LD D, u8
            RegDE.hi = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD D, u8"<<std::endl;
        }
        break;
        case 0x17: {
            // RLA 
            // Flags: 000C
            rla();
            std::cout<<"RLA "<<std::endl;
        }
        break;
        case 0x18: {
            // JR i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            programCounter += jumpBy;
            std::cout<<"JR i8"<<std::endl;
        }
        break;
        case 0x19: {
            // ADD HL, DE
            // Flags: -0HC
            RegHL.reg += RegDE.reg;
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(RegHL.reg, RegDE.reg));
            setFlag(FLAG_C, RegHL.reg > 0xffff);
            std::cout<<"ADD HL, DE"<<std::endl;
        }
        break;
        case 0x1a: {
            // LD A, (DE)
            RegAF.hi = memory->readByte(RegDE.reg);
            std::cout<<"LD A, (DE)"<<std::endl;
        }
        break;
        case 0x1b: {
            // DEC DE
            RegDE.reg--;
            std::cout<<"DEC DE"<<std::endl;
        }
        break;
        case 0x1c: {
            // INC E
            // Flags: Z0H-
            RegDE.lo++;
            if(RegDE.lo == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegDE.lo, 1));
            std::cout<<"INC E"<<std::endl;
        }
        break;
        case 0x1d: {
            // DEC E
            // Flags: Z1H-
            RegDE.lo--;
            if(RegDE.lo == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegDE.lo, 1));
            std::cout<<"DEC E"<<std::endl;
        }
        break;
        case 0x1e: {
            // LD E, u8
            RegDE.lo = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD E, u8"<<std::endl;
        }
        break;
        case 0x1f: {
            // RRA 
            // Flags: 000C
            rra();
            std::cout<<"RRA "<<std::endl;
        }
        break;
        case 0x20: {
            // JR NZ, i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(!getFlag(FLAG_Z)){
                programCounter += jumpBy;
            }
            std::cout<<"JR NZ, i8"<<std::endl;
        }
        break;
        case 0x21: {
            // LD HL, u16
            RegHL.reg = memory->readWord(programCounter);
            programCounter += 2;
            std::cout<<"LD HL, u16"<<std::endl;
        }
        break;
        case 0x22: {
            // LD (HL+), A
            memory->writeByte(RegHL.reg++, RegAF.hi);
            std::cout<<"LD (HL+), A"<<std::endl;
        }
        break;
        case 0x23: {
            // INC HL
            RegHL.reg++;
            std::cout<<"INC HL"<<std::endl;
        }
        break;
        case 0x24: {
            // INC H
            // Flags: Z0H-
            RegHL.hi++;
            if(RegHL.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegHL.hi, 1));
            std::cout<<"INC H"<<std::endl;
        }
        break;
        case 0x25: {
            // DEC H
            // Flags: Z1H-
            RegHL.hi--;
            if(RegHL.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegHL.hi, 1));
            std::cout<<"DEC H"<<std::endl;
        }
        break;
        case 0x26: {
            // LD H, u8
            RegHL.hi = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD H, u8"<<std::endl;
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
            std::cout<<"DAA "<<std::endl;
        }
        break;
        case 0x28: {
            // JR Z, i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(getFlag(FLAG_Z)){
                programCounter += jumpBy;
            }
            std::cout<<"JR Z, i8"<<std::endl;
        }
        break;
        case 0x29: {
            // ADD HL, HL
            // Flags: -0HC
            RegHL.reg += RegHL.reg;
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(RegHL.reg, RegHL.reg));
            setFlag(FLAG_C, RegHL.reg > 0xffff);
            std::cout<<"ADD HL, HL"<<std::endl;
        }
        break;
        case 0x2a: {
            // LD A, (HL+)
            RegAF.hi = memory->readByte(RegHL.reg++);
            std::cout<<"LD A, (HL+)"<<std::endl;
        }
        break;
        case 0x2b: {
            // DEC HL
            RegHL.reg--;
            std::cout<<"DEC HL"<<std::endl;
        }
        break;
        case 0x2c: {
            // INC L
            // Flags: Z0H-
            RegHL.lo++;
            if(RegHL.lo == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegHL.lo, 1));
            std::cout<<"INC L"<<std::endl;
        }
        break;
        case 0x2d: {
            // DEC L
            // Flags: Z1H-
            RegHL.lo--;
            if(RegHL.lo == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegHL.lo, 1));
            std::cout<<"DEC L"<<std::endl;
        }
        break;
        case 0x2e: {
            // LD L, u8
            RegHL.lo = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD L, u8"<<std::endl;
        }
        break;
        case 0x2f: {
            // CPL 
            // Flags: -11-
            RegAF.hi = ~RegAF.hi;
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, 1);
            std::cout<<"CPL "<<std::endl;
        }
        break;
        case 0x30: {
            // JR NC, i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(!getFlag(FLAG_C)){
                programCounter += jumpBy;
            }
            std::cout<<"JR NC, i8"<<std::endl;
        }
        break;
        case 0x31: {
            // LD SP, u16
            StackPointer.reg = memory->readWord(programCounter);
            programCounter += 2;
            std::cout<<"LD SP, u16"<<std::endl;
        }
        break;
        case 0x32: {
            // LD (HL-), A
            memory->writeByte(RegHL.reg--, RegAF.hi);
            std::cout<<"LD (HL-), A"<<std::endl;
        }
        break;
        case 0x33: {
            // INC SP
            StackPointer.reg++;
            std::cout<<"INC SP"<<std::endl;
        }
        break;
        case 0x34: {
            // INC (HL)
            // Flags: Z0H-
            memory->writeWord(memory->readWord(RegHL.reg), RegHL.reg + 1);
            if(memory->readByte(RegHL.reg) == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(memory->readByte(RegHL.reg), 1));
            std::cout<<"INC (HL)"<<std::endl;
        }
        break;
        case 0x35: {
            // DEC (HL)
            // Flags: Z1H-
            memory->writeWord(memory->readWord(RegHL.reg), RegHL.reg - 1);
            if(memory->readByte(RegHL.reg) == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(memory->readByte(RegHL.reg), 1));
            std::cout<<"DEC (HL)"<<std::endl;
        }
        break;
        case 0x36: {
            // LD (HL), u8
            memory->writeWord(RegHL.reg, memory->readByte(programCounter));
            programCounter++;
            std::cout<<"LD (HL), u8"<<std::endl;
        }
        break;
        case 0x37: {
            // SCF 
            // Flags: -001
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 1);
            std::cout<<"SCF "<<std::endl;
        }
        break;
        case 0x38: {
            // JR C, i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(getFlag(FLAG_C)){
                programCounter += jumpBy;
            }
            std::cout<<"JR C, i8"<<std::endl;
        }
        break;
        case 0x39: {
            // ADD HL, SP
            // Flags: -0HC
            RegHL.reg += StackPointer.reg;
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(RegHL.reg, StackPointer.reg));
            setFlag(FLAG_C, RegHL.reg > 0xffff);
            std::cout<<"ADD HL, SP"<<std::endl;
        }
        break;
        case 0x3a: {
            // LD A, (HL-)
            RegAF.hi = memory->readByte(RegHL.reg--);
            std::cout<<"LD A, (HL-)"<<std::endl;
        }
        break;
        case 0x3b: {
            // DEC SP
            StackPointer.reg--;
            std::cout<<"DEC SP"<<std::endl;
        }
        break;
        case 0x3c: {
            // INC A
            // Flags: Z0H-
            RegAF.hi++;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, 1));
            std::cout<<"INC A"<<std::endl;
        }
        break;
        case 0x3d: {
            // DEC A
            // Flags: Z1H-
            RegAF.hi--;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, 1));
            std::cout<<"DEC A"<<std::endl;
        }
        break;
        case 0x3e: {
            // LD A, u8
            RegAF.hi = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD A, u8"<<std::endl;
        }
        break;
        case 0x3f: {
            // CCF 
            // Flags: -00C
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, !getFlag(FLAG_C));
            std::cout<<"CCF "<<std::endl;
        }
        break;
        case 0x40: {
            // LD B, B
            RegBC.hi = RegBC.hi;
            std::cout<<"LD B, B"<<std::endl;
        }
        break;
        case 0x41: {
            // LD B, C
            RegBC.hi = RegBC.lo;
            std::cout<<"LD B, C"<<std::endl;
        }
        break;
        case 0x42: {
            // LD B, D
            RegBC.hi = RegDE.hi;
            std::cout<<"LD B, D"<<std::endl;
        }
        break;
        case 0x43: {
            // LD B, E
            RegBC.hi = RegDE.lo;
            std::cout<<"LD B, E"<<std::endl;
        }
        break;
        case 0x44: {
            // LD B, H
            RegBC.hi = RegHL.hi;
            std::cout<<"LD B, H"<<std::endl;
        }
        break;
        case 0x45: {
            // LD B, L
            RegBC.hi = RegHL.lo;
            std::cout<<"LD B, L"<<std::endl;
        }
        break;
        case 0x46: {
            // LD B, (HL)
            RegBC.hi = memory->readByte(RegHL.reg);
            std::cout<<"LD B, (HL)"<<std::endl;
        }
        break;
        case 0x47: {
            // LD B, A
            RegBC.hi = RegAF.hi;
            std::cout<<"LD B, A"<<std::endl;
        }
        break;
        case 0x48: {
            // LD C, B
            RegBC.lo = RegBC.hi;
            std::cout<<"LD C, B"<<std::endl;
        }
        break;
        case 0x49: {
            // LD C, C
            RegBC.lo = RegBC.lo;
            std::cout<<"LD C, C"<<std::endl;
        }
        break;
        case 0x4a: {
            // LD C, D
            RegBC.lo = RegDE.hi;
            std::cout<<"LD C, D"<<std::endl;
        }
        break;
        case 0x4b: {
            // LD C, E
            RegBC.lo = RegDE.lo;
            std::cout<<"LD C, E"<<std::endl;
        }
        break;
        case 0x4c: {
            // LD C, H
            RegBC.lo = RegHL.hi;
            std::cout<<"LD C, H"<<std::endl;
        }
        break;
        case 0x4d: {
            // LD C, L
            RegBC.lo = RegHL.lo;
            std::cout<<"LD C, L"<<std::endl;
        }
        break;
        case 0x4e: {
            // LD C, (HL)
            RegBC.lo = memory->readByte(RegHL.reg);
            std::cout<<"LD C, (HL)"<<std::endl;
        }
        break;
        case 0x4f: {
            // LD C, A
            RegBC.lo = RegAF.hi;
            std::cout<<"LD C, A"<<std::endl;
        }
        break;
        case 0x50: {
            // LD D, B
            RegDE.hi = RegBC.hi;
            std::cout<<"LD D, B"<<std::endl;
        }
        break;
        case 0x51: {
            // LD D, C
            RegDE.hi = RegBC.lo;
            std::cout<<"LD D, C"<<std::endl;
        }
        break;
        case 0x52: {
            // LD D, D
            RegDE.hi = RegDE.hi;
            std::cout<<"LD D, D"<<std::endl;
        }
        break;
        case 0x53: {
            // LD D, E
            RegDE.hi = RegDE.lo;
            std::cout<<"LD D, E"<<std::endl;
        }
        break;
        case 0x54: {
            // LD D, H
            RegDE.hi = RegHL.hi;
            std::cout<<"LD D, H"<<std::endl;
        }
        break;
        case 0x55: {
            // LD D, L
            RegDE.hi = RegHL.lo;
            std::cout<<"LD D, L"<<std::endl;
        }
        break;
        case 0x56: {
            // LD D, (HL)
            RegDE.hi = memory->readByte(RegHL.reg);
            std::cout<<"LD D, (HL)"<<std::endl;
        }
        break;
        case 0x57: {
            // LD D, A
            RegDE.hi = RegAF.hi;
            std::cout<<"LD D, A"<<std::endl;
        }
        break;
        case 0x58: {
            // LD E, B
            RegDE.lo = RegBC.hi;
            std::cout<<"LD E, B"<<std::endl;
        }
        break;
        case 0x59: {
            // LD E, C
            RegDE.lo = RegBC.lo;
            std::cout<<"LD E, C"<<std::endl;
        }
        break;
        case 0x5a: {
            // LD E, D
            RegDE.lo = RegDE.hi;
            std::cout<<"LD E, D"<<std::endl;
        }
        break;
        case 0x5b: {
            // LD E, E
            RegDE.lo = RegDE.lo;
            std::cout<<"LD E, E"<<std::endl;
        }
        break;
        case 0x5c: {
            // LD E, H
            RegDE.lo = RegHL.hi;
            std::cout<<"LD E, H"<<std::endl;
        }
        break;
        case 0x5d: {
            // LD E, L
            RegDE.lo = RegHL.lo;
            std::cout<<"LD E, L"<<std::endl;
        }
        break;
        case 0x5e: {
            // LD E, (HL)
            RegDE.lo = memory->readByte(RegHL.reg);
            std::cout<<"LD E, (HL)"<<std::endl;
        }
        break;
        case 0x5f: {
            // LD E, A
            RegDE.lo = RegAF.hi;
            std::cout<<"LD E, A"<<std::endl;
        }
        break;
        case 0x60: {
            // LD H, B
            RegHL.hi = RegBC.hi;
            std::cout<<"LD H, B"<<std::endl;
        }
        break;
        case 0x61: {
            // LD H, C
            RegHL.hi = RegBC.lo;
            std::cout<<"LD H, C"<<std::endl;
        }
        break;
        case 0x62: {
            // LD H, D
            RegHL.hi = RegDE.hi;
            std::cout<<"LD H, D"<<std::endl;
        }
        break;
        case 0x63: {
            // LD H, E
            RegHL.hi = RegDE.lo;
            std::cout<<"LD H, E"<<std::endl;
        }
        break;
        case 0x64: {
            // LD H, H
            RegHL.hi = RegHL.hi;
            std::cout<<"LD H, H"<<std::endl;
        }
        break;
        case 0x65: {
            // LD H, L
            RegHL.hi = RegHL.lo;
            std::cout<<"LD H, L"<<std::endl;
        }
        break;
        case 0x66: {
            // LD H, (HL)
            RegHL.hi = memory->readByte(RegHL.reg);
            std::cout<<"LD H, (HL)"<<std::endl;
        }
        break;
        case 0x67: {
            // LD H, A
            RegHL.hi = RegAF.hi;
            std::cout<<"LD H, A"<<std::endl;
        }
        break;
        case 0x68: {
            // LD L, B
            RegHL.lo = RegBC.hi;
            std::cout<<"LD L, B"<<std::endl;
        }
        break;
        case 0x69: {
            // LD L, C
            RegHL.lo = RegBC.lo;
            std::cout<<"LD L, C"<<std::endl;
        }
        break;
        case 0x6a: {
            // LD L, D
            RegHL.lo = RegDE.hi;
            std::cout<<"LD L, D"<<std::endl;
        }
        break;
        case 0x6b: {
            // LD L, E
            RegHL.lo = RegDE.lo;
            std::cout<<"LD L, E"<<std::endl;
        }
        break;
        case 0x6c: {
            // LD L, H
            RegHL.lo = RegHL.hi;
            std::cout<<"LD L, H"<<std::endl;
        }
        break;
        case 0x6d: {
            // LD L, L
            RegHL.lo = RegHL.lo;
            std::cout<<"LD L, L"<<std::endl;
        }
        break;
        case 0x6e: {
            // LD L, (HL)
            RegHL.lo = memory->readByte(RegHL.reg);
            std::cout<<"LD L, (HL)"<<std::endl;
        }
        break;
        case 0x6f: {
            // LD L, A
            RegHL.lo = RegAF.hi;
            std::cout<<"LD L, A"<<std::endl;
        }
        break;
        case 0x70: {
            // LD (HL), B
            memory->writeByte(RegHL.reg, RegBC.hi);
            std::cout<<"LD (HL), B"<<std::endl;
        }
        break;
        case 0x71: {
            // LD (HL), C
            memory->writeByte(RegHL.reg, RegBC.lo);
            std::cout<<"LD (HL), C"<<std::endl;
        }
        break;
        case 0x72: {
            // LD (HL), D
            memory->writeByte(RegHL.reg, RegDE.hi);
            std::cout<<"LD (HL), D"<<std::endl;
        }
        break;
        case 0x73: {
            // LD (HL), E
            memory->writeByte(RegHL.reg, RegDE.lo);
            std::cout<<"LD (HL), E"<<std::endl;
        }
        break;
        case 0x74: {
            // LD (HL), H
            memory->writeByte(RegHL.reg, RegHL.hi);
            std::cout<<"LD (HL), H"<<std::endl;
        }
        break;
        case 0x75: {
            // LD (HL), L
            memory->writeByte(RegHL.reg, RegHL.lo);
            std::cout<<"LD (HL), L"<<std::endl;
        }
        break;
        case 0x76: {
            // HALT 
            std::cout<<"HALT "<<std::endl;
        }
        break;
        case 0x77: {
            // LD (HL), A
            memory->writeByte(RegHL.reg, RegAF.hi);
            std::cout<<"LD (HL), A"<<std::endl;
        }
        break;
        case 0x78: {
            // LD A, B
            RegAF.hi = RegBC.hi;
            std::cout<<"LD A, B"<<std::endl;
        }
        break;
        case 0x79: {
            // LD A, C
            RegAF.hi = RegBC.lo;
            std::cout<<"LD A, C"<<std::endl;
        }
        break;
        case 0x7a: {
            // LD A, D
            RegAF.hi = RegDE.hi;
            std::cout<<"LD A, D"<<std::endl;
        }
        break;
        case 0x7b: {
            // LD A, E
            RegAF.hi = RegDE.lo;
            std::cout<<"LD A, E"<<std::endl;
        }
        break;
        case 0x7c: {
            // LD A, H
            RegAF.hi = RegHL.hi;
            std::cout<<"LD A, H"<<std::endl;
        }
        break;
        case 0x7d: {
            // LD A, L
            RegAF.hi = RegHL.lo;
            std::cout<<"LD A, L"<<std::endl;
        }
        break;
        case 0x7e: {
            // LD A, (HL)
            RegAF.hi = memory->readByte(RegHL.reg);
            std::cout<<"LD A, (HL)"<<std::endl;
        }
        break;
        case 0x7f: {
            // LD A, A
            RegAF.hi = RegAF.hi;
            std::cout<<"LD A, A"<<std::endl;
        }
        break;
        case 0x80: {
            // ADD A, B
            // Flags: Z0HC
            RegAF.hi += RegBC.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegBC.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, B"<<std::endl;
        }
        break;
        case 0x81: {
            // ADD A, C
            // Flags: Z0HC
            RegAF.hi += RegBC.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegBC.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, C"<<std::endl;
        }
        break;
        case 0x82: {
            // ADD A, D
            // Flags: Z0HC
            RegAF.hi += RegDE.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegDE.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, D"<<std::endl;
        }
        break;
        case 0x83: {
            // ADD A, E
            // Flags: Z0HC
            RegAF.hi += RegDE.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegDE.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, E"<<std::endl;
        }
        break;
        case 0x84: {
            // ADD A, H
            // Flags: Z0HC
            RegAF.hi += RegHL.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegHL.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, H"<<std::endl;
        }
        break;
        case 0x85: {
            // ADD A, L
            // Flags: Z0HC
            RegAF.hi += RegHL.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegHL.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, L"<<std::endl;
        }
        break;
        case 0x86: {
            // ADD A, (HL)
            // Flags: Z0HC
            RegAF.hi += memory->readByte(RegHL.reg);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, memory->readByte(RegHL.reg)));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, (HL)"<<std::endl;
        }
        break;
        case 0x87: {
            // ADD A, A
            // Flags: Z0HC
            RegAF.hi += RegAF.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegAF.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, A"<<std::endl;
        }
        break;
        case 0x88: {
            // ADC A, B
            // Flags: Z0HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (RegBC.hi + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegBC.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, B"<<std::endl;
        }
        break;
        case 0x89: {
            // ADC A, C
            // Flags: Z0HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (RegBC.lo + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegBC.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, C"<<std::endl;
        }
        break;
        case 0x8a: {
            // ADC A, D
            // Flags: Z0HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (RegDE.hi + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegDE.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, D"<<std::endl;
        }
        break;
        case 0x8b: {
            // ADC A, E
            // Flags: Z0HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (RegDE.lo + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegDE.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, E"<<std::endl;
        }
        break;
        case 0x8c: {
            // ADC A, H
            // Flags: Z0HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (RegHL.hi + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegHL.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, H"<<std::endl;
        }
        break;
        case 0x8d: {
            // ADC A, L
            // Flags: Z0HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (RegHL.lo + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegHL.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, L"<<std::endl;
        }
        break;
        case 0x8e: {
            // ADC A, (HL)
            // Flags: Z0HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (memory->readByte(RegHL.reg) + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, memory->readByte(RegHL.reg)));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, (HL)"<<std::endl;
        }
        break;
        case 0x8f: {
            // ADC A, A
            // Flags: Z0HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (RegAF.hi + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegAF.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, A"<<std::endl;
        }
        break;
        case 0x90: {
            // SUB A, B
            // Flags: Z1HC
            RegAF.hi -= RegBC.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegBC.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, B"<<std::endl;
        }
        break;
        case 0x91: {
            // SUB A, C
            // Flags: Z1HC
            RegAF.hi -= RegBC.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegBC.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, C"<<std::endl;
        }
        break;
        case 0x92: {
            // SUB A, D
            // Flags: Z1HC
            RegAF.hi -= RegDE.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegDE.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, D"<<std::endl;
        }
        break;
        case 0x93: {
            // SUB A, E
            // Flags: Z1HC
            RegAF.hi -= RegDE.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegDE.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, E"<<std::endl;
        }
        break;
        case 0x94: {
            // SUB A, H
            // Flags: Z1HC
            RegAF.hi -= RegHL.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegHL.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, H"<<std::endl;
        }
        break;
        case 0x95: {
            // SUB A, L
            // Flags: Z1HC
            RegAF.hi -= RegHL.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegHL.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, L"<<std::endl;
        }
        break;
        case 0x96: {
            // SUB A, (HL)
            // Flags: Z1HC
            RegAF.hi -= memory->readByte(RegHL.reg);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, memory->readByte(RegHL.reg)));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, (HL)"<<std::endl;
        }
        break;
        case 0x97: {
            // SUB A, A
            // Flags: Z1HC
            RegAF.hi -= RegAF.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegAF.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, A"<<std::endl;
        }
        break;
        case 0x98: {
            // SBC A, B
            // Flags: Z1HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (RegBC.hi - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegBC.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, B"<<std::endl;
        }
        break;
        case 0x99: {
            // SBC A, C
            // Flags: Z1HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (RegBC.lo - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegBC.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, C"<<std::endl;
        }
        break;
        case 0x9a: {
            // SBC A, D
            // Flags: Z1HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (RegDE.hi - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegDE.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, D"<<std::endl;
        }
        break;
        case 0x9b: {
            // SBC A, E
            // Flags: Z1HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (RegDE.lo - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegDE.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, E"<<std::endl;
        }
        break;
        case 0x9c: {
            // SBC A, H
            // Flags: Z1HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (RegHL.hi - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegHL.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, H"<<std::endl;
        }
        break;
        case 0x9d: {
            // SBC A, L
            // Flags: Z1HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (RegHL.lo - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegHL.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, L"<<std::endl;
        }
        break;
        case 0x9e: {
            // SBC A, (HL)
            // Flags: Z1HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (memory->readByte(RegHL.reg) - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, memory->readByte(RegHL.reg)));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, (HL)"<<std::endl;
        }
        break;
        case 0x9f: {
            // SBC A, A
            // Flags: Z1HC
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (RegAF.hi - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, RegAF.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, A"<<std::endl;
        }
        break;
        case 0xa0: {
            // AND A, B
            // Flags: Z010
            RegAF.hi &= RegBC.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, B"<<std::endl;
        }
        break;
        case 0xa1: {
            // AND A, C
            // Flags: Z010
            RegAF.hi &= RegBC.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, C"<<std::endl;
        }
        break;
        case 0xa2: {
            // AND A, D
            // Flags: Z010
            RegAF.hi &= RegDE.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, D"<<std::endl;
        }
        break;
        case 0xa3: {
            // AND A, E
            // Flags: Z010
            RegAF.hi &= RegDE.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, E"<<std::endl;
        }
        break;
        case 0xa4: {
            // AND A, H
            // Flags: Z010
            RegAF.hi &= RegHL.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, H"<<std::endl;
        }
        break;
        case 0xa5: {
            // AND A, L
            // Flags: Z010
            RegAF.hi &= RegHL.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, L"<<std::endl;
        }
        break;
        case 0xa6: {
            // AND A, (HL)
            // Flags: Z010
            RegAF.hi &= memory->readByte(RegHL.reg);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, (HL)"<<std::endl;
        }
        break;
        case 0xa7: {
            // AND A, A
            // Flags: Z010
            RegAF.hi &= RegAF.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, A"<<std::endl;
        }
        break;
        case 0xa8: {
            // XOR A, B
            // Flags: Z000
            RegAF.hi ^= RegBC.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, B"<<std::endl;
        }
        break;
        case 0xa9: {
            // XOR A, C
            // Flags: Z000
            RegAF.hi ^= RegBC.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, C"<<std::endl;
        }
        break;
        case 0xaa: {
            // XOR A, D
            // Flags: Z000
            RegAF.hi ^= RegDE.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, D"<<std::endl;
        }
        break;
        case 0xab: {
            // XOR A, E
            // Flags: Z000
            RegAF.hi ^= RegDE.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, E"<<std::endl;
        }
        break;
        case 0xac: {
            // XOR A, H
            // Flags: Z000
            RegAF.hi ^= RegHL.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, H"<<std::endl;
        }
        break;
        case 0xad: {
            // XOR A, L
            // Flags: Z000
            RegAF.hi ^= RegHL.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, L"<<std::endl;
        }
        break;
        case 0xae: {
            // XOR A, (HL)
            // Flags: Z000
            RegAF.hi ^= memory->readByte(RegHL.reg);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, (HL)"<<std::endl;
        }
        break;
        case 0xaf: {
            // XOR A, A
            // Flags: Z000
            RegAF.hi ^= RegAF.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, A"<<std::endl;
        }
        break;
        case 0xb0: {
            // OR A, B
            // Flags: Z000
            RegAF.hi |= RegBC.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, B"<<std::endl;
        }
        break;
        case 0xb1: {
            // OR A, C
            // Flags: Z000
            RegAF.hi |= RegBC.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, C"<<std::endl;
        }
        break;
        case 0xb2: {
            // OR A, D
            // Flags: Z000
            RegAF.hi |= RegDE.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, D"<<std::endl;
        }
        break;
        case 0xb3: {
            // OR A, E
            // Flags: Z000
            RegAF.hi |= RegDE.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, E"<<std::endl;
        }
        break;
        case 0xb4: {
            // OR A, H
            // Flags: Z000
            RegAF.hi |= RegHL.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, H"<<std::endl;
        }
        break;
        case 0xb5: {
            // OR A, L
            // Flags: Z000
            RegAF.hi |= RegHL.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, L"<<std::endl;
        }
        break;
        case 0xb6: {
            // OR A, (HL)
            // Flags: Z000
            RegAF.hi |= memory->readByte(RegHL.reg);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, (HL)"<<std::endl;
        }
        break;
        case 0xb7: {
            // OR A, A
            // Flags: Z000
            RegAF.hi |= RegAF.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, A"<<std::endl;
        }
        break;
        case 0xb8: {
            // CP A, B
            // Flags: Z1HC
            uint8_t result = RegAF.hi - RegBC.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, RegBC.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, B"<<std::endl;
        }
        break;
        case 0xb9: {
            // CP A, C
            // Flags: Z1HC
            uint8_t result = RegAF.hi - RegBC.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, RegBC.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, C"<<std::endl;
        }
        break;
        case 0xba: {
            // CP A, D
            // Flags: Z1HC
            uint8_t result = RegAF.hi - RegDE.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, RegDE.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, D"<<std::endl;
        }
        break;
        case 0xbb: {
            // CP A, E
            // Flags: Z1HC
            uint8_t result = RegAF.hi - RegDE.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, RegDE.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, E"<<std::endl;
        }
        break;
        case 0xbc: {
            // CP A, H
            // Flags: Z1HC
            uint8_t result = RegAF.hi - RegHL.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, RegHL.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, H"<<std::endl;
        }
        break;
        case 0xbd: {
            // CP A, L
            // Flags: Z1HC
            uint8_t result = RegAF.hi - RegHL.lo;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, RegHL.lo));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, L"<<std::endl;
        }
        break;
        case 0xbe: {
            // CP A, (HL)
            // Flags: Z1HC
            uint8_t result = RegAF.hi - memory->readByte(RegHL.reg);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, memory->readByte(RegHL.reg)));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, (HL)"<<std::endl;
        }
        break;
        case 0xbf: {
            // CP A, A
            // Flags: Z1HC
            uint8_t result = RegAF.hi - RegAF.hi;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, RegAF.hi));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, A"<<std::endl;
        }
        break;
        case 0xc0: {
            // RET NZ
            if(!getFlag(FLAG_Z)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
            }
            std::cout<<"RET NZ"<<std::endl;
        }
        break;
        case 0xc1: {
            // POP BC
            RegBC.reg = memory->readWord(StackPointer.reg);
            std::cout<<"POP BC"<<std::endl;
        }
        break;
        case 0xc2: {
            // JP NZ, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_Z)){
                programCounter = newAddress;
            }
            std::cout<<"JP NZ, u16"<<std::endl;
        }
        break;
        case 0xc3: {
            // JP u16
            programCounter = memory->readWord(programCounter);
            std::cout<<"JP u16"<<std::endl;
        }
        break;
        case 0xc4: {
            // CALL NZ, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_Z)){
                StackPointer.reg--;
                memory->writeWord(StackPointer.reg, programCounter);
                programCounter = newAddress;
            }
            std::cout<<"CALL NZ, u16"<<std::endl;
        }
        break;
        case 0xc5: {
            // PUSH BC
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, RegBC.reg);
            StackPointer.reg--;
            std::cout<<"PUSH BC"<<std::endl;
        }
        break;
        case 0xc6: {
            // ADD A, u8
            // Flags: Z0HC
            uint8_t data = memory->readByte(programCounter++);
            RegAF.hi += data;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, data));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADD A, u8"<<std::endl;
        }
        break;
        case 0xc7: {
            // RST 00h
            uint8_t val = memory->readByte(0xc7);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 00h"<<std::endl;
        }
        break;
        case 0xc8: {
            // RET Z
            if(getFlag(FLAG_Z)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
            }
            std::cout<<"RET Z"<<std::endl;
        }
        break;
        case 0xc9: {
            // RET 
            programCounter = memory->readWord(StackPointer.reg);
            StackPointer.reg += 2;
            std::cout<<"RET "<<std::endl;
        }
        break;
        case 0xca: {
            // JP Z, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_Z)){
                programCounter = newAddress;
            }
            std::cout<<"JP Z, u16"<<std::endl;
        }
        break;
        case 0xcb: {
            // PREFIX CB
            executePrefixOP(memory->readByte(programCounter++));
            std::cout<<"PREFIX CB"<<std::endl;
        }
        break;
        case 0xcc: {
            // CALL Z, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_Z)){
                StackPointer.reg--;
                memory->writeWord(StackPointer.reg, programCounter);
                programCounter = newAddress;
            }
            std::cout<<"CALL Z, u16"<<std::endl;
        }
        break;
        case 0xcd: {
            // CALL u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = newAddress;
            std::cout<<"CALL u16"<<std::endl;
        }
        break;
        case 0xce: {
            // ADC A, u8
            // Flags: Z0HC
            uint8_t data = memory->readByte(programCounter++);
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi += (data + carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, data));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"ADC A, u8"<<std::endl;
        }
        break;
        case 0xcf: {
            // RST 08h
            uint8_t val = memory->readByte(0xcf);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 08h"<<std::endl;
        }
        break;
        case 0xd0: {
            // RET NC
            if(!getFlag(FLAG_C)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
            }
            std::cout<<"RET NC"<<std::endl;
        }
        break;
        case 0xd1: {
            // POP DE
            RegDE.reg = memory->readWord(StackPointer.reg);
            std::cout<<"POP DE"<<std::endl;
        }
        break;
        case 0xd2: {
            // JP NC, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_C)){
                programCounter = newAddress;
            }
            std::cout<<"JP NC, u16"<<std::endl;
        }
        break;
        case 0xd4: {
            // CALL NC, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_C)){
                StackPointer.reg--;
                memory->writeWord(StackPointer.reg, programCounter);
                programCounter = newAddress;
            }
            std::cout<<"CALL NC, u16"<<std::endl;
        }
        break;
        case 0xd5: {
            // PUSH DE
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, RegDE.reg);
            StackPointer.reg--;
            std::cout<<"PUSH DE"<<std::endl;
        }
        break;
        case 0xd6: {
            // SUB A, u8
            // Flags: Z1HC
            uint8_t data = memory->readByte(programCounter++);
            RegAF.hi -= memory->readByte(programCounter++);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, data));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SUB A, u8"<<std::endl;
        }
        break;
        case 0xd7: {
            // RST 10h
            uint8_t val = memory->readByte(0xd7);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 10h"<<std::endl;
        }
        break;
        case 0xd8: {
            // RET C
            if(getFlag(FLAG_C)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
            }
            std::cout<<"RET C"<<std::endl;
        }
        break;
        case 0xd9: {
            // RETI 
            std::cout<<"RETI "<<std::endl;
        }
        break;
        case 0xda: {
            // JP C, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_C)){
                programCounter = newAddress;
            }
            std::cout<<"JP C, u16"<<std::endl;
        }
        break;
        case 0xdc: {
            // CALL C, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_C)){
                StackPointer.reg--;
                memory->writeWord(StackPointer.reg, programCounter);
                programCounter = newAddress;
            }
            std::cout<<"CALL C, u16"<<std::endl;
        }
        break;
        case 0xde: {
            // SBC A, u8
            // Flags: Z1HC
            uint8_t data = memory->readByte(programCounter++);
            uint8_t carry = getFlag(FLAG_C);
            RegAF.hi -= (data - carry);
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(RegAF.hi, data));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"SBC A, u8"<<std::endl;
        }
        break;
        case 0xdf: {
            // RST 18h
            uint8_t val = memory->readByte(0xdf);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 18h"<<std::endl;
        }
        break;
        case 0xe0: {
            // LD (FF00+u8), A
            memory->writeByte(0xFF00 + memory->readByte(programCounter), RegAF.hi);
            programCounter++;
            std::cout<<"LD (FF00+u8), A"<<std::endl;
        }
        break;
        case 0xe1: {
            // POP HL
            RegHL.reg = memory->readWord(StackPointer.reg);
            std::cout<<"POP HL"<<std::endl;
        }
        break;
        case 0xe2: {
            // LD (FF00+C), A
            memory->writeByte(0xFF00 + RegBC.lo, RegAF.hi);
            std::cout<<"LD (FF00+C), A"<<std::endl;
        }
        break;
        case 0xe5: {
            // PUSH HL
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, RegHL.reg);
            StackPointer.reg--;
            std::cout<<"PUSH HL"<<std::endl;
        }
        break;
        case 0xe6: {
            // AND A, u8
            // Flags: Z010
            uint8_t data = memory->readByte(programCounter++);
            RegAF.hi &= data;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 1);
            setFlag(FLAG_C, 0);
            std::cout<<"AND A, u8"<<std::endl;
        }
        break;
        case 0xe7: {
            // RST 20h
            uint8_t val = memory->readByte(0xe7);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 20h"<<std::endl;
        }
        break;
        case 0xe8: {
            // ADD SP, i8
            // Flags: 00HC
            int8_t data = (int8_t) memory->readByte(programCounter++);
            StackPointer.reg += data;
            setFlag(FLAG_Z, 0);
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(StackPointer.reg, data));
            setFlag(FLAG_C, StackPointer.reg > 0xffff);
            std::cout<<"ADD SP, i8"<<std::endl;
        }
        break;
        case 0xe9: {
            // JP HL
            programCounter = RegHL.reg;
            std::cout<<"JP HL"<<std::endl;
        }
        break;
        case 0xea: {
            // LD (u16), A
            memory->writeByte(memory->readWord(programCounter), RegAF.hi);
            programCounter += 2;
            std::cout<<"LD (u16), A"<<std::endl;
        }
        break;
        case 0xee: {
            // XOR A, u8
            // Flags: Z000
            uint8_t data = memory->readByte(programCounter++);
            RegAF.hi ^= data;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"XOR A, u8"<<std::endl;
        }
        break;
        case 0xef: {
            // RST 28h
            uint8_t val = memory->readByte(0xef);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 28h"<<std::endl;
        }
        break;
        case 0xf0: {
            // LD A, (FF00+u8)
            RegAF.hi = memory->readByte(0xFF00 + memory->readByte(programCounter));
            programCounter++;
            std::cout<<"LD A, (FF00+u8)"<<std::endl;
        }
        break;
        case 0xf1: {
            // POP AF
            // Flags: ZNHC
            RegAF.reg = memory->readWord(StackPointer.reg);
            // lower nibble of F register must be reset
            RegAF.lo &= 0xF0;
            std::cout<<"POP AF"<<std::endl;
        }
        break;
        case 0xf2: {
            // LD A, (FF00+C)
            RegAF.hi = memory->readByte(0xFF00 + RegBC.lo);
            std::cout<<"LD A, (FF00+C)"<<std::endl;
        }
        break;
        case 0xf3: {
            // DI 
            std::cout<<"DI "<<std::endl;
        }
        break;
        case 0xf5: {
            // PUSH AF
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, RegAF.reg);
            StackPointer.reg--;
            std::cout<<"PUSH AF"<<std::endl;
        }
        break;
        case 0xf6: {
            // OR A, u8
            // Flags: Z000
            uint8_t data = memory->readByte(programCounter++);
            RegAF.hi |= data;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 0);
            std::cout<<"OR A, u8"<<std::endl;
        }
        break;
        case 0xf7: {
            // RST 30h
            uint8_t val = memory->readByte(0xf7);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 30h"<<std::endl;
        }
        break;
        case 0xf8: {
            // LD HL, SP+i8
            // Flags: 00HC
            RegHL.reg = StackPointer.reg + (int8_t) memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD HL, SP+i8"<<std::endl;
        }
        break;
        case 0xf9: {
            // LD SP, HL
            StackPointer.reg = RegHL.reg;
            std::cout<<"LD SP, HL"<<std::endl;
        }
        break;
        case 0xfa: {
            // LD A, (u16)
            RegAF.hi = memory->readByte(programCounter);
            programCounter += 2;
            std::cout<<"LD A, (u16)"<<std::endl;
        }
        break;
        case 0xfb: {
            // EI 
            std::cout<<"EI "<<std::endl;
        }
        break;
        case 0xfe: {
            // CP A, u8
            // Flags: Z1HC
            uint8_t data = memory->readByte(programCounter++);
            uint8_t result = RegAF.hi - data;
            if(RegAF.hi == 0){
                setFlag(FLAG_Z, 1);
            }
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, halfCarry8(result, data));
            setFlag(FLAG_C, RegAF.hi > 0xff);
            std::cout<<"CP A, u8"<<std::endl;
        }
        break;
        case 0xff: {
            // RST 38h
            uint8_t val = memory->readByte(0xff);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 38h"<<std::endl;
        }
        break;
        default:
            std::cout << "invalid or unimplemented op code" << std::endl;
            break;
    }
}

void CPU::executePrefixOP(uint8_t opCode){
    switch(opCode){
        case 0x00: {
            // RLC B
            // Flags: Z00C
            rlc(RegBC.hi);
            std::cout<<"RLC B"<<std::endl;
        }
        break;
        case 0x01: {
            // RLC C
            // Flags: Z00C
            rlc(RegBC.lo);
            std::cout<<"RLC C"<<std::endl;
        }
        break;
        case 0x02: {
            // RLC D
            // Flags: Z00C
            rlc(RegDE.hi);
            std::cout<<"RLC D"<<std::endl;
        }
        break;
        case 0x03: {
            // RLC E
            // Flags: Z00C
            rlc(RegDE.lo);
            std::cout<<"RLC E"<<std::endl;
        }
        break;
        case 0x04: {
            // RLC H
            // Flags: Z00C
            rlc(RegHL.hi);
            std::cout<<"RLC H"<<std::endl;
        }
        break;
        case 0x05: {
            // RLC L
            // Flags: Z00C
            rlc(RegHL.lo);
            std::cout<<"RLC L"<<std::endl;
        }
        break;
        case 0x06: {
            // RLC (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            rlc(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RLC (HL)"<<std::endl;
        }
        break;
        case 0x07: {
            // RLC A
            // Flags: Z00C
            rlc(RegAF.hi);
            std::cout<<"RLC A"<<std::endl;
        }
        break;
        case 0x08: {
            // RRC B
            // Flags: Z00C
            rrc(RegBC.hi);
            std::cout<<"RRC B"<<std::endl;
        }
        break;
        case 0x09: {
            // RRC C
            // Flags: Z00C
            rrc(RegBC.lo);
            std::cout<<"RRC C"<<std::endl;
        }
        break;
        case 0x0a: {
            // RRC D
            // Flags: Z00C
            rrc(RegDE.hi);
            std::cout<<"RRC D"<<std::endl;
        }
        break;
        case 0x0b: {
            // RRC E
            // Flags: Z00C
            rrc(RegDE.lo);
            std::cout<<"RRC E"<<std::endl;
        }
        break;
        case 0x0c: {
            // RRC H
            // Flags: Z00C
            rrc(RegHL.hi);
            std::cout<<"RRC H"<<std::endl;
        }
        break;
        case 0x0d: {
            // RRC L
            // Flags: Z00C
            rrc(RegHL.lo);
            std::cout<<"RRC L"<<std::endl;
        }
        break;
        case 0x0e: {
            // RRC (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            rrc(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RRC (HL)"<<std::endl;
        }
        break;
        case 0x0f: {
            // RRC A
            // Flags: Z00C
            rrc(RegAF.hi);
            std::cout<<"RRC A"<<std::endl;
        }
        break;
        case 0x10: {
            // RL B
            // Flags: Z00C
            rl(RegBC.hi);
            std::cout<<"RL B"<<std::endl;
        }
        break;
        case 0x11: {
            // RL C
            // Flags: Z00C
            rl(RegBC.lo);
            std::cout<<"RL C"<<std::endl;
        }
        break;
        case 0x12: {
            // RL D
            // Flags: Z00C
            rl(RegDE.hi);
            std::cout<<"RL D"<<std::endl;
        }
        break;
        case 0x13: {
            // RL E
            // Flags: Z00C
            rl(RegDE.lo);
            std::cout<<"RL E"<<std::endl;
        }
        break;
        case 0x14: {
            // RL H
            // Flags: Z00C
            rl(RegHL.hi);
            std::cout<<"RL H"<<std::endl;
        }
        break;
        case 0x15: {
            // RL L
            // Flags: Z00C
            rl(RegHL.lo);
            std::cout<<"RL L"<<std::endl;
        }
        break;
        case 0x16: {
            // RL (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            rl(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RL (HL)"<<std::endl;
        }
        break;
        case 0x17: {
            // RL A
            // Flags: Z00C
            rl(RegAF.hi);
            std::cout<<"RL A"<<std::endl;
        }
        break;
        case 0x18: {
            // RR B
            // Flags: Z00C
            rr(RegBC.hi);
            std::cout<<"RR B"<<std::endl;
        }
        break;
        case 0x19: {
            // RR C
            // Flags: Z00C
            rr(RegBC.lo);
            std::cout<<"RR C"<<std::endl;
        }
        break;
        case 0x1a: {
            // RR D
            // Flags: Z00C
            rr(RegDE.hi);
            std::cout<<"RR D"<<std::endl;
        }
        break;
        case 0x1b: {
            // RR E
            // Flags: Z00C
            rr(RegDE.lo);
            std::cout<<"RR E"<<std::endl;
        }
        break;
        case 0x1c: {
            // RR H
            // Flags: Z00C
            rr(RegHL.hi);
            std::cout<<"RR H"<<std::endl;
        }
        break;
        case 0x1d: {
            // RR L
            // Flags: Z00C
            rr(RegHL.lo);
            std::cout<<"RR L"<<std::endl;
        }
        break;
        case 0x1e: {
            // RR (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            rr(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RR (HL)"<<std::endl;
        }
        break;
        case 0x1f: {
            // RR A
            // Flags: Z00C
            rr(RegAF.hi);
            std::cout<<"RR A"<<std::endl;
        }
        break;
        case 0x20: {
            // SLA B
            // Flags: Z00C
            sla(RegBC.hi);
            std::cout<<"SLA B"<<std::endl;
        }
        break;
        case 0x21: {
            // SLA C
            // Flags: Z00C
            sla(RegBC.lo);
            std::cout<<"SLA C"<<std::endl;
        }
        break;
        case 0x22: {
            // SLA D
            // Flags: Z00C
            sla(RegDE.hi);
            std::cout<<"SLA D"<<std::endl;
        }
        break;
        case 0x23: {
            // SLA E
            // Flags: Z00C
            sla(RegDE.lo);
            std::cout<<"SLA E"<<std::endl;
        }
        break;
        case 0x24: {
            // SLA H
            // Flags: Z00C
            sla(RegHL.hi);
            std::cout<<"SLA H"<<std::endl;
        }
        break;
        case 0x25: {
            // SLA L
            // Flags: Z00C
            sla(RegHL.lo);
            std::cout<<"SLA L"<<std::endl;
        }
        break;
        case 0x26: {
            // SLA (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            sla(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SLA (HL)"<<std::endl;
        }
        break;
        case 0x27: {
            // SLA A
            // Flags: Z00C
            sla(RegAF.hi);
            std::cout<<"SLA A"<<std::endl;
        }
        break;
        case 0x28: {
            // SRA B
            // Flags: Z00C
            sra(RegBC.hi);
            std::cout<<"SRA B"<<std::endl;
        }
        break;
        case 0x29: {
            // SRA C
            // Flags: Z00C
            sra(RegBC.lo);
            std::cout<<"SRA C"<<std::endl;
        }
        break;
        case 0x2a: {
            // SRA D
            // Flags: Z00C
            sra(RegDE.hi);
            std::cout<<"SRA D"<<std::endl;
        }
        break;
        case 0x2b: {
            // SRA E
            // Flags: Z00C
            sra(RegDE.lo);
            std::cout<<"SRA E"<<std::endl;
        }
        break;
        case 0x2c: {
            // SRA H
            // Flags: Z00C
            sra(RegHL.hi);
            std::cout<<"SRA H"<<std::endl;
        }
        break;
        case 0x2d: {
            // SRA L
            // Flags: Z00C
            sra(RegHL.lo);
            std::cout<<"SRA L"<<std::endl;
        }
        break;
        case 0x2e: {
            // SRA (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            sra(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SRA (HL)"<<std::endl;
        }
        break;
        case 0x2f: {
            // SRA A
            // Flags: Z00C
            sra(RegAF.hi);
            std::cout<<"SRA A"<<std::endl;
        }
        break;
        case 0x30: {
            // SWAP B
            // Flags: Z000
            swap(RegBC.hi);
            std::cout<<"SWAP B"<<std::endl;
        }
        break;
        case 0x31: {
            // SWAP C
            // Flags: Z000
            swap(RegBC.lo);
            std::cout<<"SWAP C"<<std::endl;
        }
        break;
        case 0x32: {
            // SWAP D
            // Flags: Z000
            swap(RegDE.hi);
            std::cout<<"SWAP D"<<std::endl;
        }
        break;
        case 0x33: {
            // SWAP E
            // Flags: Z000
            swap(RegDE.lo);
            std::cout<<"SWAP E"<<std::endl;
        }
        break;
        case 0x34: {
            // SWAP H
            // Flags: Z000
            swap(RegHL.hi);
            std::cout<<"SWAP H"<<std::endl;
        }
        break;
        case 0x35: {
            // SWAP L
            // Flags: Z000
            swap(RegHL.lo);
            std::cout<<"SWAP L"<<std::endl;
        }
        break;
        case 0x36: {
            // SWAP (HL)
            // Flags: Z000
            uint8_t hlVal = memory->readByte(RegHL.reg);
            swap(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SWAP (HL)"<<std::endl;
        }
        break;
        case 0x37: {
            // SWAP A
            // Flags: Z000
            swap(RegAF.hi);
            std::cout<<"SWAP A"<<std::endl;
        }
        break;
        case 0x38: {
            // SRL B
            // Flags: Z00C
            srl(RegBC.hi);
            std::cout<<"SRL B"<<std::endl;
        }
        break;
        case 0x39: {
            // SRL C
            // Flags: Z00C
            srl(RegBC.lo);
            std::cout<<"SRL C"<<std::endl;
        }
        break;
        case 0x3a: {
            // SRL D
            // Flags: Z00C
            srl(RegDE.hi);
            std::cout<<"SRL D"<<std::endl;
        }
        break;
        case 0x3b: {
            // SRL E
            // Flags: Z00C
            srl(RegDE.lo);
            std::cout<<"SRL E"<<std::endl;
        }
        break;
        case 0x3c: {
            // SRL H
            // Flags: Z00C
            srl(RegHL.hi);
            std::cout<<"SRL H"<<std::endl;
        }
        break;
        case 0x3d: {
            // SRL L
            // Flags: Z00C
            srl(RegHL.lo);
            std::cout<<"SRL L"<<std::endl;
        }
        break;
        case 0x3e: {
            // SRL (HL)
            // Flags: Z00C
            uint8_t hlVal = memory->readByte(RegHL.reg);
            srl(hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SRL (HL)"<<std::endl;
        }
        break;
        case 0x3f: {
            // SRL A
            // Flags: Z00C
            srl(RegAF.hi);
            std::cout<<"SRL A"<<std::endl;
        }
        break;
        case 0x40: {
            // BIT 0, B
            // Flags: Z01-
            bit(0, RegBC.hi);
            std::cout<<"BIT 0, B"<<std::endl;
        }
        break;
        case 0x41: {
            // BIT 0, C
            // Flags: Z01-
            bit(0, RegBC.lo);
            std::cout<<"BIT 0, C"<<std::endl;
        }
        break;
        case 0x42: {
            // BIT 0, D
            // Flags: Z01-
            bit(0, RegDE.hi);
            std::cout<<"BIT 0, D"<<std::endl;
        }
        break;
        case 0x43: {
            // BIT 0, E
            // Flags: Z01-
            bit(0, RegDE.lo);
            std::cout<<"BIT 0, E"<<std::endl;
        }
        break;
        case 0x44: {
            // BIT 0, H
            // Flags: Z01-
            bit(0, RegHL.hi);
            std::cout<<"BIT 0, H"<<std::endl;
        }
        break;
        case 0x45: {
            // BIT 0, L
            // Flags: Z01-
            bit(0, RegHL.lo);
            std::cout<<"BIT 0, L"<<std::endl;
        }
        break;
        case 0x46: {
            // BIT 0, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(0, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"BIT 0, (HL)"<<std::endl;
        }
        break;
        case 0x47: {
            // BIT 0, A
            // Flags: Z01-
            bit(0, RegAF.hi);
            std::cout<<"BIT 0, A"<<std::endl;
        }
        break;
        case 0x48: {
            // BIT 1, B
            // Flags: Z01-
            bit(1, RegBC.hi);
            std::cout<<"BIT 1, B"<<std::endl;
        }
        break;
        case 0x49: {
            // BIT 1, C
            // Flags: Z01-
            bit(1, RegBC.lo);
            std::cout<<"BIT 1, C"<<std::endl;
        }
        break;
        case 0x4a: {
            // BIT 1, D
            // Flags: Z01-
            bit(1, RegDE.hi);
            std::cout<<"BIT 1, D"<<std::endl;
        }
        break;
        case 0x4b: {
            // BIT 1, E
            // Flags: Z01-
            bit(1, RegDE.lo);
            std::cout<<"BIT 1, E"<<std::endl;
        }
        break;
        case 0x4c: {
            // BIT 1, H
            // Flags: Z01-
            bit(1, RegHL.hi);
            std::cout<<"BIT 1, H"<<std::endl;
        }
        break;
        case 0x4d: {
            // BIT 1, L
            // Flags: Z01-
            bit(1, RegHL.lo);
            std::cout<<"BIT 1, L"<<std::endl;
        }
        break;
        case 0x4e: {
            // BIT 1, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(1, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"BIT 1, (HL)"<<std::endl;
        }
        break;
        case 0x4f: {
            // BIT 1, A
            // Flags: Z01-
            bit(1, RegAF.hi);
            std::cout<<"BIT 1, A"<<std::endl;
        }
        break;
        case 0x50: {
            // BIT 2, B
            // Flags: Z01-
            bit(2, RegBC.hi);
            std::cout<<"BIT 2, B"<<std::endl;
        }
        break;
        case 0x51: {
            // BIT 2, C
            // Flags: Z01-
            bit(2, RegBC.lo);
            std::cout<<"BIT 2, C"<<std::endl;
        }
        break;
        case 0x52: {
            // BIT 2, D
            // Flags: Z01-
            bit(2, RegDE.hi);
            std::cout<<"BIT 2, D"<<std::endl;
        }
        break;
        case 0x53: {
            // BIT 2, E
            // Flags: Z01-
            bit(2, RegDE.lo);
            std::cout<<"BIT 2, E"<<std::endl;
        }
        break;
        case 0x54: {
            // BIT 2, H
            // Flags: Z01-
            bit(2, RegHL.hi);
            std::cout<<"BIT 2, H"<<std::endl;
        }
        break;
        case 0x55: {
            // BIT 2, L
            // Flags: Z01-
            bit(2, RegHL.lo);
            std::cout<<"BIT 2, L"<<std::endl;
        }
        break;
        case 0x56: {
            // BIT 2, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(2, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"BIT 2, (HL)"<<std::endl;
        }
        break;
        case 0x57: {
            // BIT 2, A
            // Flags: Z01-
            bit(2, RegAF.hi);
            std::cout<<"BIT 2, A"<<std::endl;
        }
        break;
        case 0x58: {
            // BIT 3, B
            // Flags: Z01-
            bit(3, RegBC.hi);
            std::cout<<"BIT 3, B"<<std::endl;
        }
        break;
        case 0x59: {
            // BIT 3, C
            // Flags: Z01-
            bit(3, RegBC.lo);
            std::cout<<"BIT 3, C"<<std::endl;
        }
        break;
        case 0x5a: {
            // BIT 3, D
            // Flags: Z01-
            bit(3, RegDE.hi);
            std::cout<<"BIT 3, D"<<std::endl;
        }
        break;
        case 0x5b: {
            // BIT 3, E
            // Flags: Z01-
            bit(3, RegDE.lo);
            std::cout<<"BIT 3, E"<<std::endl;
        }
        break;
        case 0x5c: {
            // BIT 3, H
            // Flags: Z01-
            bit(3, RegHL.hi);
            std::cout<<"BIT 3, H"<<std::endl;
        }
        break;
        case 0x5d: {
            // BIT 3, L
            // Flags: Z01-
            bit(3, RegHL.lo);
            std::cout<<"BIT 3, L"<<std::endl;
        }
        break;
        case 0x5e: {
            // BIT 3, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(3, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"BIT 3, (HL)"<<std::endl;
        }
        break;
        case 0x5f: {
            // BIT 3, A
            // Flags: Z01-
            bit(3, RegAF.hi);
            std::cout<<"BIT 3, A"<<std::endl;
        }
        break;
        case 0x60: {
            // BIT 4, B
            // Flags: Z01-
            bit(4, RegBC.hi);
            std::cout<<"BIT 4, B"<<std::endl;
        }
        break;
        case 0x61: {
            // BIT 4, C
            // Flags: Z01-
            bit(4, RegBC.lo);
            std::cout<<"BIT 4, C"<<std::endl;
        }
        break;
        case 0x62: {
            // BIT 4, D
            // Flags: Z01-
            bit(4, RegDE.hi);
            std::cout<<"BIT 4, D"<<std::endl;
        }
        break;
        case 0x63: {
            // BIT 4, E
            // Flags: Z01-
            bit(4, RegDE.lo);
            std::cout<<"BIT 4, E"<<std::endl;
        }
        break;
        case 0x64: {
            // BIT 4, H
            // Flags: Z01-
            bit(4, RegHL.hi);
            std::cout<<"BIT 4, H"<<std::endl;
        }
        break;
        case 0x65: {
            // BIT 4, L
            // Flags: Z01-
            bit(4, RegHL.lo);
            std::cout<<"BIT 4, L"<<std::endl;
        }
        break;
        case 0x66: {
            // BIT 4, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(4, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"BIT 4, (HL)"<<std::endl;
        }
        break;
        case 0x67: {
            // BIT 4, A
            // Flags: Z01-
            bit(4, RegAF.hi);
            std::cout<<"BIT 4, A"<<std::endl;
        }
        break;
        case 0x68: {
            // BIT 5, B
            // Flags: Z01-
            bit(5, RegBC.hi);
            std::cout<<"BIT 5, B"<<std::endl;
        }
        break;
        case 0x69: {
            // BIT 5, C
            // Flags: Z01-
            bit(5, RegBC.lo);
            std::cout<<"BIT 5, C"<<std::endl;
        }
        break;
        case 0x6a: {
            // BIT 5, D
            // Flags: Z01-
            bit(5, RegDE.hi);
            std::cout<<"BIT 5, D"<<std::endl;
        }
        break;
        case 0x6b: {
            // BIT 5, E
            // Flags: Z01-
            bit(5, RegDE.lo);
            std::cout<<"BIT 5, E"<<std::endl;
        }
        break;
        case 0x6c: {
            // BIT 5, H
            // Flags: Z01-
            bit(5, RegHL.hi);
            std::cout<<"BIT 5, H"<<std::endl;
        }
        break;
        case 0x6d: {
            // BIT 5, L
            // Flags: Z01-
            bit(5, RegHL.lo);
            std::cout<<"BIT 5, L"<<std::endl;
        }
        break;
        case 0x6e: {
            // BIT 5, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(5, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"BIT 5, (HL)"<<std::endl;
        }
        break;
        case 0x6f: {
            // BIT 5, A
            // Flags: Z01-
            bit(5, RegAF.hi);
            std::cout<<"BIT 5, A"<<std::endl;
        }
        break;
        case 0x70: {
            // BIT 6, B
            // Flags: Z01-
            bit(6, RegBC.hi);
            std::cout<<"BIT 6, B"<<std::endl;
        }
        break;
        case 0x71: {
            // BIT 6, C
            // Flags: Z01-
            bit(6, RegBC.lo);
            std::cout<<"BIT 6, C"<<std::endl;
        }
        break;
        case 0x72: {
            // BIT 6, D
            // Flags: Z01-
            bit(6, RegDE.hi);
            std::cout<<"BIT 6, D"<<std::endl;
        }
        break;
        case 0x73: {
            // BIT 6, E
            // Flags: Z01-
            bit(6, RegDE.lo);
            std::cout<<"BIT 6, E"<<std::endl;
        }
        break;
        case 0x74: {
            // BIT 6, H
            // Flags: Z01-
            bit(6, RegHL.hi);
            std::cout<<"BIT 6, H"<<std::endl;
        }
        break;
        case 0x75: {
            // BIT 6, L
            // Flags: Z01-
            bit(6, RegHL.lo);
            std::cout<<"BIT 6, L"<<std::endl;
        }
        break;
        case 0x76: {
            // BIT 6, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(6, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"BIT 6, (HL)"<<std::endl;
        }
        break;
        case 0x77: {
            // BIT 6, A
            // Flags: Z01-
            bit(6, RegAF.hi);
            std::cout<<"BIT 6, A"<<std::endl;
        }
        break;
        case 0x78: {
            // BIT 7, B
            // Flags: Z01-
            bit(7, RegBC.hi);
            std::cout<<"BIT 7, B"<<std::endl;
        }
        break;
        case 0x79: {
            // BIT 7, C
            // Flags: Z01-
            bit(7, RegBC.lo);
            std::cout<<"BIT 7, C"<<std::endl;
        }
        break;
        case 0x7a: {
            // BIT 7, D
            // Flags: Z01-
            bit(7, RegDE.hi);
            std::cout<<"BIT 7, D"<<std::endl;
        }
        break;
        case 0x7b: {
            // BIT 7, E
            // Flags: Z01-
            bit(7, RegDE.lo);
            std::cout<<"BIT 7, E"<<std::endl;
        }
        break;
        case 0x7c: {
            // BIT 7, H
            // Flags: Z01-
            bit(7, RegHL.hi);
            std::cout<<"BIT 7, H"<<std::endl;
        }
        break;
        case 0x7d: {
            // BIT 7, L
            // Flags: Z01-
            bit(7, RegHL.lo);
            std::cout<<"BIT 7, L"<<std::endl;
        }
        break;
        case 0x7e: {
            // BIT 7, (HL)
            // Flags: Z01-
            uint8_t hlVal = memory->readByte(RegHL.reg);
            bit(7, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"BIT 7, (HL)"<<std::endl;
        }
        break;
        case 0x7f: {
            // BIT 7, A
            // Flags: Z01-
            bit(7, RegAF.hi);
            std::cout<<"BIT 7, A"<<std::endl;
        }
        break;
        case 0x80: {
            // RES 0, B
            res(0, RegBC.hi);
            std::cout<<"RES 0, B"<<std::endl;
        }
        break;
        case 0x81: {
            // RES 0, C
            res(0, RegBC.lo);
            std::cout<<"RES 0, C"<<std::endl;
        }
        break;
        case 0x82: {
            // RES 0, D
            res(0, RegDE.hi);
            std::cout<<"RES 0, D"<<std::endl;
        }
        break;
        case 0x83: {
            // RES 0, E
            res(0, RegDE.lo);
            std::cout<<"RES 0, E"<<std::endl;
        }
        break;
        case 0x84: {
            // RES 0, H
            res(0, RegHL.hi);
            std::cout<<"RES 0, H"<<std::endl;
        }
        break;
        case 0x85: {
            // RES 0, L
            res(0, RegHL.lo);
            std::cout<<"RES 0, L"<<std::endl;
        }
        break;
        case 0x86: {
            // RES 0, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(0, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RES 0, (HL)"<<std::endl;
        }
        break;
        case 0x87: {
            // RES 0, A
            res(0, RegAF.hi);
            std::cout<<"RES 0, A"<<std::endl;
        }
        break;
        case 0x88: {
            // RES 1, B
            res(1, RegBC.hi);
            std::cout<<"RES 1, B"<<std::endl;
        }
        break;
        case 0x89: {
            // RES 1, C
            res(1, RegBC.lo);
            std::cout<<"RES 1, C"<<std::endl;
        }
        break;
        case 0x8a: {
            // RES 1, D
            res(1, RegDE.hi);
            std::cout<<"RES 1, D"<<std::endl;
        }
        break;
        case 0x8b: {
            // RES 1, E
            res(1, RegDE.lo);
            std::cout<<"RES 1, E"<<std::endl;
        }
        break;
        case 0x8c: {
            // RES 1, H
            res(1, RegHL.hi);
            std::cout<<"RES 1, H"<<std::endl;
        }
        break;
        case 0x8d: {
            // RES 1, L
            res(1, RegHL.lo);
            std::cout<<"RES 1, L"<<std::endl;
        }
        break;
        case 0x8e: {
            // RES 1, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(1, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RES 1, (HL)"<<std::endl;
        }
        break;
        case 0x8f: {
            // RES 1, A
            res(1, RegAF.hi);
            std::cout<<"RES 1, A"<<std::endl;
        }
        break;
        case 0x90: {
            // RES 2, B
            res(2, RegBC.hi);
            std::cout<<"RES 2, B"<<std::endl;
        }
        break;
        case 0x91: {
            // RES 2, C
            res(2, RegBC.lo);
            std::cout<<"RES 2, C"<<std::endl;
        }
        break;
        case 0x92: {
            // RES 2, D
            res(2, RegDE.hi);
            std::cout<<"RES 2, D"<<std::endl;
        }
        break;
        case 0x93: {
            // RES 2, E
            res(2, RegDE.lo);
            std::cout<<"RES 2, E"<<std::endl;
        }
        break;
        case 0x94: {
            // RES 2, H
            res(2, RegHL.hi);
            std::cout<<"RES 2, H"<<std::endl;
        }
        break;
        case 0x95: {
            // RES 2, L
            res(2, RegHL.lo);
            std::cout<<"RES 2, L"<<std::endl;
        }
        break;
        case 0x96: {
            // RES 2, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(2, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RES 2, (HL)"<<std::endl;
        }
        break;
        case 0x97: {
            // RES 2, A
            res(2, RegAF.hi);
            std::cout<<"RES 2, A"<<std::endl;
        }
        break;
        case 0x98: {
            // RES 3, B
            res(3, RegBC.hi);
            std::cout<<"RES 3, B"<<std::endl;
        }
        break;
        case 0x99: {
            // RES 3, C
            res(3, RegBC.lo);
            std::cout<<"RES 3, C"<<std::endl;
        }
        break;
        case 0x9a: {
            // RES 3, D
            res(3, RegDE.hi);
            std::cout<<"RES 3, D"<<std::endl;
        }
        break;
        case 0x9b: {
            // RES 3, E
            res(3, RegDE.lo);
            std::cout<<"RES 3, E"<<std::endl;
        }
        break;
        case 0x9c: {
            // RES 3, H
            res(3, RegHL.hi);
            std::cout<<"RES 3, H"<<std::endl;
        }
        break;
        case 0x9d: {
            // RES 3, L
            res(3, RegHL.lo);
            std::cout<<"RES 3, L"<<std::endl;
        }
        break;
        case 0x9e: {
            // RES 3, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(3, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RES 3, (HL)"<<std::endl;
        }
        break;
        case 0x9f: {
            // RES 3, A
            res(3, RegAF.hi);
            std::cout<<"RES 3, A"<<std::endl;
        }
        break;
        case 0xa0: {
            // RES 4, B
            res(4, RegBC.hi);
            std::cout<<"RES 4, B"<<std::endl;
        }
        break;
        case 0xa1: {
            // RES 4, C
            res(4, RegBC.lo);
            std::cout<<"RES 4, C"<<std::endl;
        }
        break;
        case 0xa2: {
            // RES 4, D
            res(4, RegDE.hi);
            std::cout<<"RES 4, D"<<std::endl;
        }
        break;
        case 0xa3: {
            // RES 4, E
            res(4, RegDE.lo);
            std::cout<<"RES 4, E"<<std::endl;
        }
        break;
        case 0xa4: {
            // RES 4, H
            res(4, RegHL.hi);
            std::cout<<"RES 4, H"<<std::endl;
        }
        break;
        case 0xa5: {
            // RES 4, L
            res(4, RegHL.lo);
            std::cout<<"RES 4, L"<<std::endl;
        }
        break;
        case 0xa6: {
            // RES 4, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(4, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RES 4, (HL)"<<std::endl;
        }
        break;
        case 0xa7: {
            // RES 4, A
            res(4, RegAF.hi);
            std::cout<<"RES 4, A"<<std::endl;
        }
        break;
        case 0xa8: {
            // RES 5, B
            res(5, RegBC.hi);
            std::cout<<"RES 5, B"<<std::endl;
        }
        break;
        case 0xa9: {
            // RES 5, C
            res(5, RegBC.lo);
            std::cout<<"RES 5, C"<<std::endl;
        }
        break;
        case 0xaa: {
            // RES 5, D
            res(5, RegDE.hi);
            std::cout<<"RES 5, D"<<std::endl;
        }
        break;
        case 0xab: {
            // RES 5, E
            res(5, RegDE.lo);
            std::cout<<"RES 5, E"<<std::endl;
        }
        break;
        case 0xac: {
            // RES 5, H
            res(5, RegHL.hi);
            std::cout<<"RES 5, H"<<std::endl;
        }
        break;
        case 0xad: {
            // RES 5, L
            res(5, RegHL.lo);
            std::cout<<"RES 5, L"<<std::endl;
        }
        break;
        case 0xae: {
            // RES 5, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(5, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RES 5, (HL)"<<std::endl;
        }
        break;
        case 0xaf: {
            // RES 5, A
            res(5, RegAF.hi);
            std::cout<<"RES 5, A"<<std::endl;
        }
        break;
        case 0xb0: {
            // RES 6, B
            res(6, RegBC.hi);
            std::cout<<"RES 6, B"<<std::endl;
        }
        break;
        case 0xb1: {
            // RES 6, C
            res(6, RegBC.lo);
            std::cout<<"RES 6, C"<<std::endl;
        }
        break;
        case 0xb2: {
            // RES 6, D
            res(6, RegDE.hi);
            std::cout<<"RES 6, D"<<std::endl;
        }
        break;
        case 0xb3: {
            // RES 6, E
            res(6, RegDE.lo);
            std::cout<<"RES 6, E"<<std::endl;
        }
        break;
        case 0xb4: {
            // RES 6, H
            res(6, RegHL.hi);
            std::cout<<"RES 6, H"<<std::endl;
        }
        break;
        case 0xb5: {
            // RES 6, L
            res(6, RegHL.lo);
            std::cout<<"RES 6, L"<<std::endl;
        }
        break;
        case 0xb6: {
            // RES 6, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(6, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RES 6, (HL)"<<std::endl;
        }
        break;
        case 0xb7: {
            // RES 6, A
            res(6, RegAF.hi);
            std::cout<<"RES 6, A"<<std::endl;
        }
        break;
        case 0xb8: {
            // RES 7, B
            res(7, RegBC.hi);
            std::cout<<"RES 7, B"<<std::endl;
        }
        break;
        case 0xb9: {
            // RES 7, C
            res(7, RegBC.lo);
            std::cout<<"RES 7, C"<<std::endl;
        }
        break;
        case 0xba: {
            // RES 7, D
            res(7, RegDE.hi);
            std::cout<<"RES 7, D"<<std::endl;
        }
        break;
        case 0xbb: {
            // RES 7, E
            res(7, RegDE.lo);
            std::cout<<"RES 7, E"<<std::endl;
        }
        break;
        case 0xbc: {
            // RES 7, H
            res(7, RegHL.hi);
            std::cout<<"RES 7, H"<<std::endl;
        }
        break;
        case 0xbd: {
            // RES 7, L
            res(7, RegHL.lo);
            std::cout<<"RES 7, L"<<std::endl;
        }
        break;
        case 0xbe: {
            // RES 7, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            res(7, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"RES 7, (HL)"<<std::endl;
        }
        break;
        case 0xbf: {
            // RES 7, A
            res(7, RegAF.hi);
            std::cout<<"RES 7, A"<<std::endl;
        }
        break;
        case 0xc0: {
            // SET 0, B
            set(0, RegBC.hi);
            std::cout<<"SET 0, B"<<std::endl;
        }
        break;
        case 0xc1: {
            // SET 0, C
            set(0, RegBC.lo);
            std::cout<<"SET 0, C"<<std::endl;
        }
        break;
        case 0xc2: {
            // SET 0, D
            set(0, RegDE.hi);
            std::cout<<"SET 0, D"<<std::endl;
        }
        break;
        case 0xc3: {
            // SET 0, E
            set(0, RegDE.lo);
            std::cout<<"SET 0, E"<<std::endl;
        }
        break;
        case 0xc4: {
            // SET 0, H
            set(0, RegHL.hi);
            std::cout<<"SET 0, H"<<std::endl;
        }
        break;
        case 0xc5: {
            // SET 0, L
            set(0, RegHL.lo);
            std::cout<<"SET 0, L"<<std::endl;
        }
        break;
        case 0xc6: {
            // SET 0, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(0, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SET 0, (HL)"<<std::endl;
        }
        break;
        case 0xc7: {
            // SET 0, A
            set(0, RegAF.hi);
            std::cout<<"SET 0, A"<<std::endl;
        }
        break;
        case 0xc8: {
            // SET 1, B
            set(1, RegBC.hi);
            std::cout<<"SET 1, B"<<std::endl;
        }
        break;
        case 0xc9: {
            // SET 1, C
            set(1, RegBC.lo);
            std::cout<<"SET 1, C"<<std::endl;
        }
        break;
        case 0xca: {
            // SET 1, D
            set(1, RegDE.hi);
            std::cout<<"SET 1, D"<<std::endl;
        }
        break;
        case 0xcb: {
            // SET 1, E
            set(1, RegDE.lo);
            std::cout<<"SET 1, E"<<std::endl;
        }
        break;
        case 0xcc: {
            // SET 1, H
            set(1, RegHL.hi);
            std::cout<<"SET 1, H"<<std::endl;
        }
        break;
        case 0xcd: {
            // SET 1, L
            set(1, RegHL.lo);
            std::cout<<"SET 1, L"<<std::endl;
        }
        break;
        case 0xce: {
            // SET 1, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(1, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SET 1, (HL)"<<std::endl;
        }
        break;
        case 0xcf: {
            // SET 1, A
            set(1, RegAF.hi);
            std::cout<<"SET 1, A"<<std::endl;
        }
        break;
        case 0xd0: {
            // SET 2, B
            set(2, RegBC.hi);
            std::cout<<"SET 2, B"<<std::endl;
        }
        break;
        case 0xd1: {
            // SET 2, C
            set(2, RegBC.lo);
            std::cout<<"SET 2, C"<<std::endl;
        }
        break;
        case 0xd2: {
            // SET 2, D
            set(2, RegDE.hi);
            std::cout<<"SET 2, D"<<std::endl;
        }
        break;
        case 0xd3: {
            // SET 2, E
            set(2, RegDE.lo);
            std::cout<<"SET 2, E"<<std::endl;
        }
        break;
        case 0xd4: {
            // SET 2, H
            set(2, RegHL.hi);
            std::cout<<"SET 2, H"<<std::endl;
        }
        break;
        case 0xd5: {
            // SET 2, L
            set(2, RegHL.lo);
            std::cout<<"SET 2, L"<<std::endl;
        }
        break;
        case 0xd6: {
            // SET 2, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(2, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SET 2, (HL)"<<std::endl;
        }
        break;
        case 0xd7: {
            // SET 2, A
            set(2, RegAF.hi);
            std::cout<<"SET 2, A"<<std::endl;
        }
        break;
        case 0xd8: {
            // SET 3, B
            set(3, RegBC.hi);
            std::cout<<"SET 3, B"<<std::endl;
        }
        break;
        case 0xd9: {
            // SET 3, C
            set(3, RegBC.lo);
            std::cout<<"SET 3, C"<<std::endl;
        }
        break;
        case 0xda: {
            // SET 3, D
            set(3, RegDE.hi);
            std::cout<<"SET 3, D"<<std::endl;
        }
        break;
        case 0xdb: {
            // SET 3, E
            set(3, RegDE.lo);
            std::cout<<"SET 3, E"<<std::endl;
        }
        break;
        case 0xdc: {
            // SET 3, H
            set(3, RegHL.hi);
            std::cout<<"SET 3, H"<<std::endl;
        }
        break;
        case 0xdd: {
            // SET 3, L
            set(3, RegHL.lo);
            std::cout<<"SET 3, L"<<std::endl;
        }
        break;
        case 0xde: {
            // SET 3, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(3, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SET 3, (HL)"<<std::endl;
        }
        break;
        case 0xdf: {
            // SET 3, A
            set(3, RegAF.hi);
            std::cout<<"SET 3, A"<<std::endl;
        }
        break;
        case 0xe0: {
            // SET 4, B
            set(4, RegBC.hi);
            std::cout<<"SET 4, B"<<std::endl;
        }
        break;
        case 0xe1: {
            // SET 4, C
            set(4, RegBC.lo);
            std::cout<<"SET 4, C"<<std::endl;
        }
        break;
        case 0xe2: {
            // SET 4, D
            set(4, RegDE.hi);
            std::cout<<"SET 4, D"<<std::endl;
        }
        break;
        case 0xe3: {
            // SET 4, E
            set(4, RegDE.lo);
            std::cout<<"SET 4, E"<<std::endl;
        }
        break;
        case 0xe4: {
            // SET 4, H
            set(4, RegHL.hi);
            std::cout<<"SET 4, H"<<std::endl;
        }
        break;
        case 0xe5: {
            // SET 4, L
            set(4, RegHL.lo);
            std::cout<<"SET 4, L"<<std::endl;
        }
        break;
        case 0xe6: {
            // SET 4, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(4, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SET 4, (HL)"<<std::endl;
        }
        break;
        case 0xe7: {
            // SET 4, A
            set(4, RegAF.hi);
            std::cout<<"SET 4, A"<<std::endl;
        }
        break;
        case 0xe8: {
            // SET 5, B
            set(5, RegBC.hi);
            std::cout<<"SET 5, B"<<std::endl;
        }
        break;
        case 0xe9: {
            // SET 5, C
            set(5, RegBC.lo);
            std::cout<<"SET 5, C"<<std::endl;
        }
        break;
        case 0xea: {
            // SET 5, D
            set(5, RegDE.hi);
            std::cout<<"SET 5, D"<<std::endl;
        }
        break;
        case 0xeb: {
            // SET 5, E
            set(5, RegDE.lo);
            std::cout<<"SET 5, E"<<std::endl;
        }
        break;
        case 0xec: {
            // SET 5, H
            set(5, RegHL.hi);
            std::cout<<"SET 5, H"<<std::endl;
        }
        break;
        case 0xed: {
            // SET 5, L
            set(5, RegHL.lo);
            std::cout<<"SET 5, L"<<std::endl;
        }
        break;
        case 0xee: {
            // SET 5, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(5, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SET 5, (HL)"<<std::endl;
        }
        break;
        case 0xef: {
            // SET 5, A
            set(5, RegAF.hi);
            std::cout<<"SET 5, A"<<std::endl;
        }
        break;
        case 0xf0: {
            // SET 6, B
            set(6, RegBC.hi);
            std::cout<<"SET 6, B"<<std::endl;
        }
        break;
        case 0xf1: {
            // SET 6, C
            set(6, RegBC.lo);
            std::cout<<"SET 6, C"<<std::endl;
        }
        break;
        case 0xf2: {
            // SET 6, D
            set(6, RegDE.hi);
            std::cout<<"SET 6, D"<<std::endl;
        }
        break;
        case 0xf3: {
            // SET 6, E
            set(6, RegDE.lo);
            std::cout<<"SET 6, E"<<std::endl;
        }
        break;
        case 0xf4: {
            // SET 6, H
            set(6, RegHL.hi);
            std::cout<<"SET 6, H"<<std::endl;
        }
        break;
        case 0xf5: {
            // SET 6, L
            set(6, RegHL.lo);
            std::cout<<"SET 6, L"<<std::endl;
        }
        break;
        case 0xf6: {
            // SET 6, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(6, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SET 6, (HL)"<<std::endl;
        }
        break;
        case 0xf7: {
            // SET 6, A
            set(6, RegAF.hi);
            std::cout<<"SET 6, A"<<std::endl;
        }
        break;
        case 0xf8: {
            // SET 7, B
            set(7, RegBC.hi);
            std::cout<<"SET 7, B"<<std::endl;
        }
        break;
        case 0xf9: {
            // SET 7, C
            set(7, RegBC.lo);
            std::cout<<"SET 7, C"<<std::endl;
        }
        break;
        case 0xfa: {
            // SET 7, D
            set(7, RegDE.hi);
            std::cout<<"SET 7, D"<<std::endl;
        }
        break;
        case 0xfb: {
            // SET 7, E
            set(7, RegDE.lo);
            std::cout<<"SET 7, E"<<std::endl;
        }
        break;
        case 0xfc: {
            // SET 7, H
            set(7, RegHL.hi);
            std::cout<<"SET 7, H"<<std::endl;
        }
        break;
        case 0xfd: {
            // SET 7, L
            set(7, RegHL.lo);
            std::cout<<"SET 7, L"<<std::endl;
        }
        break;
        case 0xfe: {
            // SET 7, (HL)
            uint8_t hlVal = memory->readByte(RegHL.reg);
            set(7, hlVal);
            memory->writeByte(RegHL.reg, hlVal);
            std::cout<<"SET 7, (HL)"<<std::endl;
        }
        break;
        case 0xff: {
            // SET 7, A
            set(7, RegAF.hi);
            std::cout<<"SET 7, A"<<std::endl;
        }
        break;
        default:
            std::cout << "invalid or unimplemented op code" << std::endl;
            break;
    }
}