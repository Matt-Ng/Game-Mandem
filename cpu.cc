#include <iostream>
#include <fstream>

#include "cpu.hh"

/**
 * @brief 
 * Emulate the internal state of the gameboy Z80
 */

CPU::CPU(){
    programCounter = 0x100;
    RegAF.reg = 0x01B0;
    RegBC.reg = 0x0013;
    RegDE.reg = 0x00D8;
    RegHL.reg = 0x014D;

    StackPointer.reg = 0xFFFE;

    memory = new Memory();
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

void CPU::rr(uint8_t &reg){
    resetFlags();

    // set lsb as carry
    setFlag(FLAG_C, reg & 0x01);

    // rotate register right
    reg >>= 1;
}

void CPU::rl(uint8_t &reg){
    resetFlags();

    // set msb as carry
    setFlag(FLAG_C, reg & 0x80);

    // rotate register right
    reg <<= 1;
}

void CPU::nextOP(){
    executeOP(programCounter++);
}

void CPU::executeOP(u_int8_t opCode){
    switch(opCode){
        case 0x00: {
            // NOP 
            std::cout<<"NOP "<<std::endl;
        }		break;
        case 0x01: {
            // LD BC, u16
            RegBC.reg = memory->readWord(programCounter);
            programCounter += 2;
            std::cout<<"LD BC, u16"<<std::endl;
        }		break;
        case 0x02: {
            // LD (BC), A
            memory->writeByte(RegBC.reg, RegAF.hi);
            std::cout<<"LD (BC), A"<<std::endl;
        }		break;
        case 0x03: {
            // INC BC
            RegBC.reg++;
            std::cout<<"INC BC"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0x06: {
            // LD B, u8
            RegBC.hi = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD B, u8"<<std::endl;
        }		break;
        case 0x07: {
            // RLCA 
            // Flags: 000C
            rl(RegAF.hi);
            // make lsb equal to carry bit		RegAF.lo |= ((0x01) & (getFlag(FLAG_C)));
            std::cout<<"RLCA "<<std::endl;
        }		break;
        case 0x08: {
            // LD (u16), SP
            memory->writeWord(memory->readWord(programCounter), StackPointer.reg);
            programCounter += 2;
            std::cout<<"LD (u16), SP"<<std::endl;
        }		break;
        case 0x09: {
            // ADD HL, BC
            // Flags: -0HC
            RegHL.reg += RegBC.reg;
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(RegHL.reg, RegBC.reg));
            setFlag(FLAG_C, RegHL.reg > 0xffff);
            std::cout<<"ADD HL, BC"<<std::endl;
        }		break;
        case 0x0a: {
            // LD A, (BC)
            RegAF.hi = memory->readByte(RegBC.reg);
            std::cout<<"LD A, (BC)"<<std::endl;
        }		break;
        case 0x0b: {
            // DEC BC
            RegBC.reg--;
            std::cout<<"DEC BC"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0x0e: {
            // LD C, u8
            RegBC.lo = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD C, u8"<<std::endl;
        }		break;
        case 0x0f: {
            // RRCA 
            // Flags: 000C
            rr(RegAF.hi);
            // make msb equal to carry bit		RegAF.hi |= ((0x80) & (7 << getFlag(FLAG_C)));
            std::cout<<"RRCA "<<std::endl;
        }		break;
        case 0x10: {
            // STOP 
            std::cout<<"STOP "<<std::endl;
        }		break;
        case 0x11: {
            // LD DE, u16
            RegDE.reg = memory->readWord(programCounter);
            programCounter += 2;
            std::cout<<"LD DE, u16"<<std::endl;
        }		break;
        case 0x12: {
            // LD (DE), A
            memory->writeByte(RegDE.reg, RegAF.hi);
            std::cout<<"LD (DE), A"<<std::endl;
        }		break;
        case 0x13: {
            // INC DE
            RegDE.reg++;
            std::cout<<"INC DE"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0x16: {
            // LD D, u8
            RegDE.hi = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD D, u8"<<std::endl;
        }		break;
        case 0x17: {
            // RLA 
            // Flags: 000C
            rl(RegAF.hi);
            std::cout<<"RLA "<<std::endl;
        }		break;
        case 0x18: {
            // JR i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            programCounter += jumpBy;
            std::cout<<"JR i8"<<std::endl;
        }		break;
        case 0x19: {
            // ADD HL, DE
            // Flags: -0HC
            RegHL.reg += RegDE.reg;
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(RegHL.reg, RegDE.reg));
            setFlag(FLAG_C, RegHL.reg > 0xffff);
            std::cout<<"ADD HL, DE"<<std::endl;
        }		break;
        case 0x1a: {
            // LD A, (DE)
            RegAF.hi = memory->readByte(RegDE.reg);
            std::cout<<"LD A, (DE)"<<std::endl;
        }		break;
        case 0x1b: {
            // DEC DE
            RegDE.reg--;
            std::cout<<"DEC DE"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0x1e: {
            // LD E, u8
            RegDE.lo = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD E, u8"<<std::endl;
        }		break;
        case 0x1f: {
            // RRA 
            // Flags: 000C
            rr(RegAF.hi);
            std::cout<<"RRA "<<std::endl;
        }		break;
        case 0x20: {
            // JR NZ, i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(!getFlag(FLAG_Z)){
                programCounter += jumpBy;
            }
            std::cout<<"JR NZ, i8"<<std::endl;
        }		break;
        case 0x21: {
            // LD HL, u16
            RegHL.reg = memory->readWord(programCounter);
            programCounter += 2;
            std::cout<<"LD HL, u16"<<std::endl;
        }		break;
        case 0x22: {
            // LD (HL+), A
            memory->writeByte(RegHL.reg++, RegAF.hi);
            std::cout<<"LD (HL+), A"<<std::endl;
        }		break;
        case 0x23: {
            // INC HL
            RegHL.reg++;
            std::cout<<"INC HL"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0x26: {
            // LD H, u8
            RegHL.hi = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD H, u8"<<std::endl;
        }		break;
        case 0x27: {
            // DAA 
            // Flags: Z-0C
            uint8_t AReg = RegAF.hi;
            uint8_t offset = 0;
            if((!getFlag(FLAG_N) && (AReg & 0xF) > 0x09) || getFlag(FLAG_H)){
                offset |= 0x06;
            }
            if((!getFlag(FLAG_N) && (AReg & 0xF) > 0x99) || getFlag(FLAG_C)){
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
        }		break;
        case 0x28: {
            // JR Z, i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(getFlag(FLAG_Z)){
                programCounter += jumpBy;
            }
            std::cout<<"JR Z, i8"<<std::endl;
        }		break;
        case 0x29: {
            // ADD HL, HL
            // Flags: -0HC
            RegHL.reg += RegHL.reg;
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(RegHL.reg, RegHL.reg));
            setFlag(FLAG_C, RegHL.reg > 0xffff);
            std::cout<<"ADD HL, HL"<<std::endl;
        }		break;
        case 0x2a: {
            // LD A, (HL+)
            RegAF.hi = memory->readByte(RegHL.reg++);
            std::cout<<"LD A, (HL+)"<<std::endl;
        }		break;
        case 0x2b: {
            // DEC HL
            RegHL.reg--;
            std::cout<<"DEC HL"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0x2e: {
            // LD L, u8
            RegHL.lo = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD L, u8"<<std::endl;
        }		break;
        case 0x2f: {
            // CPL 
            // Flags: -11-
            RegAF.hi = ~RegAF.hi;
            setFlag(FLAG_N, 1);
            setFlag(FLAG_H, 1);
            std::cout<<"CPL "<<std::endl;
        }		break;
        case 0x30: {
            // JR NC, i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(!getFlag(FLAG_C)){
                programCounter += jumpBy;
            }
            std::cout<<"JR NC, i8"<<std::endl;
        }		break;
        case 0x31: {
            // LD SP, u16
            StackPointer.reg = memory->readWord(programCounter);
            programCounter += 2;
            std::cout<<"LD SP, u16"<<std::endl;
        }		break;
        case 0x32: {
            // LD (HL-), A
            memory->writeByte(RegHL.reg--, RegAF.hi);
            std::cout<<"LD (HL-), A"<<std::endl;
        }		break;
        case 0x33: {
            // INC SP
            StackPointer.reg++;
            std::cout<<"INC SP"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0x36: {
            // LD (HL), u8
            memory->writeWord(RegHL.reg, memory->readByte(programCounter));
            programCounter++;
            std::cout<<"LD (HL), u8"<<std::endl;
        }		break;
        case 0x37: {
            // SCF 
            // Flags: -001
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, 1);
            std::cout<<"SCF "<<std::endl;
        }		break;
        case 0x38: {
            // JR C, i8
            int8_t jumpBy = (int8_t) memory->readByte(programCounter++);
            if(getFlag(FLAG_C)){
                programCounter += jumpBy;
            }
            std::cout<<"JR C, i8"<<std::endl;
        }		break;
        case 0x39: {
            // ADD HL, SP
            // Flags: -0HC
            RegHL.reg += StackPointer.reg;
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, halfCarry16(RegHL.reg, StackPointer.reg));
            setFlag(FLAG_C, RegHL.reg > 0xffff);
            std::cout<<"ADD HL, SP"<<std::endl;
        }		break;
        case 0x3a: {
            // LD A, (HL-)
            RegAF.hi = memory->readByte(RegHL.reg--);
            std::cout<<"LD A, (HL-)"<<std::endl;
        }		break;
        case 0x3b: {
            // DEC SP
            StackPointer.reg--;
            std::cout<<"DEC SP"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0x3e: {
            // LD A, u8
            RegAF.hi = memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD A, u8"<<std::endl;
        }		break;
        case 0x3f: {
            // CCF 
            // Flags: -00C
            setFlag(FLAG_N, 0);
            setFlag(FLAG_H, 0);
            setFlag(FLAG_C, !getFlag(FLAG_C));
            std::cout<<"CCF "<<std::endl;
        }		break;
        case 0x40: {
            // LD B, B
            RegBC.hi = RegBC.hi;
            std::cout<<"LD B, B"<<std::endl;
        }		break;
        case 0x41: {
            // LD B, C
            RegBC.hi = RegBC.lo;
            std::cout<<"LD B, C"<<std::endl;
        }		break;
        case 0x42: {
            // LD B, D
            RegBC.hi = RegDE.hi;
            std::cout<<"LD B, D"<<std::endl;
        }		break;
        case 0x43: {
            // LD B, E
            RegBC.hi = RegDE.lo;
            std::cout<<"LD B, E"<<std::endl;
        }		break;
        case 0x44: {
            // LD B, H
            RegBC.hi = RegHL.hi;
            std::cout<<"LD B, H"<<std::endl;
        }		break;
        case 0x45: {
            // LD B, L
            RegBC.hi = RegHL.lo;
            std::cout<<"LD B, L"<<std::endl;
        }		break;
        case 0x46: {
            // LD B, (HL)
            RegBC.hi = memory->readByte(RegHL.reg);
            std::cout<<"LD B, (HL)"<<std::endl;
        }		break;
        case 0x47: {
            // LD B, A
            RegBC.hi = RegAF.hi;
            std::cout<<"LD B, A"<<std::endl;
        }		break;
        case 0x48: {
            // LD C, B
            RegBC.lo = RegBC.hi;
            std::cout<<"LD C, B"<<std::endl;
        }		break;
        case 0x49: {
            // LD C, C
            RegBC.lo = RegBC.lo;
            std::cout<<"LD C, C"<<std::endl;
        }		break;
        case 0x4a: {
            // LD C, D
            RegBC.lo = RegDE.hi;
            std::cout<<"LD C, D"<<std::endl;
        }		break;
        case 0x4b: {
            // LD C, E
            RegBC.lo = RegDE.lo;
            std::cout<<"LD C, E"<<std::endl;
        }		break;
        case 0x4c: {
            // LD C, H
            RegBC.lo = RegHL.hi;
            std::cout<<"LD C, H"<<std::endl;
        }		break;
        case 0x4d: {
            // LD C, L
            RegBC.lo = RegHL.lo;
            std::cout<<"LD C, L"<<std::endl;
        }		break;
        case 0x4e: {
            // LD C, (HL)
            RegBC.lo = memory->readByte(RegHL.reg);
            std::cout<<"LD C, (HL)"<<std::endl;
        }		break;
        case 0x4f: {
            // LD C, A
            RegBC.lo = RegAF.hi;
            std::cout<<"LD C, A"<<std::endl;
        }		break;
        case 0x50: {
            // LD D, B
            RegDE.hi = RegBC.hi;
            std::cout<<"LD D, B"<<std::endl;
        }		break;
        case 0x51: {
            // LD D, C
            RegDE.hi = RegBC.lo;
            std::cout<<"LD D, C"<<std::endl;
        }		break;
        case 0x52: {
            // LD D, D
            RegDE.hi = RegDE.hi;
            std::cout<<"LD D, D"<<std::endl;
        }		break;
        case 0x53: {
            // LD D, E
            RegDE.hi = RegDE.lo;
            std::cout<<"LD D, E"<<std::endl;
        }		break;
        case 0x54: {
            // LD D, H
            RegDE.hi = RegHL.hi;
            std::cout<<"LD D, H"<<std::endl;
        }		break;
        case 0x55: {
            // LD D, L
            RegDE.hi = RegHL.lo;
            std::cout<<"LD D, L"<<std::endl;
        }		break;
        case 0x56: {
            // LD D, (HL)
            RegDE.hi = memory->readByte(RegHL.reg);
            std::cout<<"LD D, (HL)"<<std::endl;
        }		break;
        case 0x57: {
            // LD D, A
            RegDE.hi = RegAF.hi;
            std::cout<<"LD D, A"<<std::endl;
        }		break;
        case 0x58: {
            // LD E, B
            RegDE.lo = RegBC.hi;
            std::cout<<"LD E, B"<<std::endl;
        }		break;
        case 0x59: {
            // LD E, C
            RegDE.lo = RegBC.lo;
            std::cout<<"LD E, C"<<std::endl;
        }		break;
        case 0x5a: {
            // LD E, D
            RegDE.lo = RegDE.hi;
            std::cout<<"LD E, D"<<std::endl;
        }		break;
        case 0x5b: {
            // LD E, E
            RegDE.lo = RegDE.lo;
            std::cout<<"LD E, E"<<std::endl;
        }		break;
        case 0x5c: {
            // LD E, H
            RegDE.lo = RegHL.hi;
            std::cout<<"LD E, H"<<std::endl;
        }		break;
        case 0x5d: {
            // LD E, L
            RegDE.lo = RegHL.lo;
            std::cout<<"LD E, L"<<std::endl;
        }		break;
        case 0x5e: {
            // LD E, (HL)
            RegDE.lo = memory->readByte(RegHL.reg);
            std::cout<<"LD E, (HL)"<<std::endl;
        }		break;
        case 0x5f: {
            // LD E, A
            RegDE.lo = RegAF.hi;
            std::cout<<"LD E, A"<<std::endl;
        }		break;
        case 0x60: {
            // LD H, B
            RegHL.hi = RegBC.hi;
            std::cout<<"LD H, B"<<std::endl;
        }		break;
        case 0x61: {
            // LD H, C
            RegHL.hi = RegBC.lo;
            std::cout<<"LD H, C"<<std::endl;
        }		break;
        case 0x62: {
            // LD H, D
            RegHL.hi = RegDE.hi;
            std::cout<<"LD H, D"<<std::endl;
        }		break;
        case 0x63: {
            // LD H, E
            RegHL.hi = RegDE.lo;
            std::cout<<"LD H, E"<<std::endl;
        }		break;
        case 0x64: {
            // LD H, H
            RegHL.hi = RegHL.hi;
            std::cout<<"LD H, H"<<std::endl;
        }		break;
        case 0x65: {
            // LD H, L
            RegHL.hi = RegHL.lo;
            std::cout<<"LD H, L"<<std::endl;
        }		break;
        case 0x66: {
            // LD H, (HL)
            RegHL.hi = memory->readByte(RegHL.reg);
            std::cout<<"LD H, (HL)"<<std::endl;
        }		break;
        case 0x67: {
            // LD H, A
            RegHL.hi = RegAF.hi;
            std::cout<<"LD H, A"<<std::endl;
        }		break;
        case 0x68: {
            // LD L, B
            RegHL.lo = RegBC.hi;
            std::cout<<"LD L, B"<<std::endl;
        }		break;
        case 0x69: {
            // LD L, C
            RegHL.lo = RegBC.lo;
            std::cout<<"LD L, C"<<std::endl;
        }		break;
        case 0x6a: {
            // LD L, D
            RegHL.lo = RegDE.hi;
            std::cout<<"LD L, D"<<std::endl;
        }		break;
        case 0x6b: {
            // LD L, E
            RegHL.lo = RegDE.lo;
            std::cout<<"LD L, E"<<std::endl;
        }		break;
        case 0x6c: {
            // LD L, H
            RegHL.lo = RegHL.hi;
            std::cout<<"LD L, H"<<std::endl;
        }		break;
        case 0x6d: {
            // LD L, L
            RegHL.lo = RegHL.lo;
            std::cout<<"LD L, L"<<std::endl;
        }		break;
        case 0x6e: {
            // LD L, (HL)
            RegHL.lo = memory->readByte(RegHL.reg);
            std::cout<<"LD L, (HL)"<<std::endl;
        }		break;
        case 0x6f: {
            // LD L, A
            RegHL.lo = RegAF.hi;
            std::cout<<"LD L, A"<<std::endl;
        }		break;
        case 0x70: {
            // LD (HL), B
            memory->writeByte(RegHL.reg, RegBC.hi);
            std::cout<<"LD (HL), B"<<std::endl;
        }		break;
        case 0x71: {
            // LD (HL), C
            memory->writeByte(RegHL.reg, RegBC.lo);
            std::cout<<"LD (HL), C"<<std::endl;
        }		break;
        case 0x72: {
            // LD (HL), D
            memory->writeByte(RegHL.reg, RegDE.hi);
            std::cout<<"LD (HL), D"<<std::endl;
        }		break;
        case 0x73: {
            // LD (HL), E
            memory->writeByte(RegHL.reg, RegDE.lo);
            std::cout<<"LD (HL), E"<<std::endl;
        }		break;
        case 0x74: {
            // LD (HL), H
            memory->writeByte(RegHL.reg, RegHL.hi);
            std::cout<<"LD (HL), H"<<std::endl;
        }		break;
        case 0x75: {
            // LD (HL), L
            memory->writeByte(RegHL.reg, RegHL.lo);
            std::cout<<"LD (HL), L"<<std::endl;
        }		break;
        case 0x76: {
            // HALT 
            std::cout<<"HALT "<<std::endl;
        }		break;
        case 0x77: {
            // LD (HL), A
            memory->writeByte(RegHL.reg, RegAF.hi);
            std::cout<<"LD (HL), A"<<std::endl;
        }		break;
        case 0x78: {
            // LD A, B
            RegAF.hi = RegBC.hi;
            std::cout<<"LD A, B"<<std::endl;
        }		break;
        case 0x79: {
            // LD A, C
            RegAF.hi = RegBC.lo;
            std::cout<<"LD A, C"<<std::endl;
        }		break;
        case 0x7a: {
            // LD A, D
            RegAF.hi = RegDE.hi;
            std::cout<<"LD A, D"<<std::endl;
        }		break;
        case 0x7b: {
            // LD A, E
            RegAF.hi = RegDE.lo;
            std::cout<<"LD A, E"<<std::endl;
        }		break;
        case 0x7c: {
            // LD A, H
            RegAF.hi = RegHL.hi;
            std::cout<<"LD A, H"<<std::endl;
        }		break;
        case 0x7d: {
            // LD A, L
            RegAF.hi = RegHL.lo;
            std::cout<<"LD A, L"<<std::endl;
        }		break;
        case 0x7e: {
            // LD A, (HL)
            RegAF.hi = memory->readByte(RegHL.reg);
            std::cout<<"LD A, (HL)"<<std::endl;
        }		break;
        case 0x7f: {
            // LD A, A
            RegAF.hi = RegAF.hi;
            std::cout<<"LD A, A"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
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
        }		break;
        case 0xc0: {
            // RET NZ
            if(!getFlag(FLAG_Z)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
            }
            std::cout<<"RET NZ"<<std::endl;
        }		break;
        case 0xc1: {
            // POP BC
            RegBC.reg = memory->readWord(StackPointer.reg);
            std::cout<<"POP BC"<<std::endl;
        }		break;
        case 0xc2: {
            // JP NZ, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_Z)){
                programCounter = newAddress;
            }
            std::cout<<"JP NZ, u16"<<std::endl;
        }		break;
        case 0xc3: {
            // JP u16
            programCounter = memory->readWord(programCounter);
            std::cout<<"JP u16"<<std::endl;
        }		break;
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
        }		break;
        case 0xc5: {
            // PUSH BC
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, RegBC.reg);
            StackPointer.reg--;
            std::cout<<"PUSH BC"<<std::endl;
        }		break;
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
        }		break;
        case 0xc7: {
            // RST 00h
            uint8_t val = memory->readByte(0xc7);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 00h"<<std::endl;
        }		break;
        case 0xc8: {
            // RET Z
            if(getFlag(FLAG_Z)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
            }
            std::cout<<"RET Z"<<std::endl;
        }		break;
        case 0xc9: {
            // RET 
            programCounter = memory->readWord(StackPointer.reg);
            StackPointer.reg += 2;
            std::cout<<"RET "<<std::endl;
        }		break;
        case 0xca: {
            // JP Z, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_Z)){
                programCounter = newAddress;
            }
            std::cout<<"JP Z, u16"<<std::endl;
        }		break;
        case 0xcb: {
            // PREFIX CB
            std::cout<<"PREFIX CB"<<std::endl;
        }		break;
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
        }		break;
        case 0xcd: {
            // CALL u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, programCounter);
            programCounter = newAddress;
            std::cout<<"CALL u16"<<std::endl;
        }		break;
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
        }		break;
        case 0xcf: {
            // RST 08h
            uint8_t val = memory->readByte(0xcf);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 08h"<<std::endl;
        }		break;
        case 0xd0: {
            // RET NC
            if(!getFlag(FLAG_C)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
            }
            std::cout<<"RET NC"<<std::endl;
        }		break;
        case 0xd1: {
            // POP DE
            RegDE.reg = memory->readWord(StackPointer.reg);
            std::cout<<"POP DE"<<std::endl;
        }		break;
        case 0xd2: {
            // JP NC, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(!getFlag(FLAG_C)){
                programCounter = newAddress;
            }
            std::cout<<"JP NC, u16"<<std::endl;
        }		break;
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
        }		break;
        case 0xd5: {
            // PUSH DE
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, RegDE.reg);
            StackPointer.reg--;
            std::cout<<"PUSH DE"<<std::endl;
        }		break;
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
        }		break;
        case 0xd7: {
            // RST 10h
            uint8_t val = memory->readByte(0xd7);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 10h"<<std::endl;
        }		break;
        case 0xd8: {
            // RET C
            if(getFlag(FLAG_C)){
                programCounter = memory->readWord(StackPointer.reg);
                StackPointer.reg += 2;
            }
            std::cout<<"RET C"<<std::endl;
        }		break;
        case 0xd9: {
            // RETI 
            std::cout<<"RETI "<<std::endl;
        }		break;
        case 0xda: {
            // JP C, u16
            uint16_t newAddress = memory->readWord(programCounter);
            programCounter += 2;
            if(getFlag(FLAG_C)){
                programCounter = newAddress;
            }
            std::cout<<"JP C, u16"<<std::endl;
        }		break;
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
        }		break;
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
        }		break;
        case 0xdf: {
            // RST 18h
            uint8_t val = memory->readByte(0xdf);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 18h"<<std::endl;
        }		break;
        case 0xe0: {
            // LD (FF00+u8), A
            memory->writeByte(0xFF00 + memory->readByte(programCounter), RegAF.hi);
            programCounter++;
            std::cout<<"LD (FF00+u8), A"<<std::endl;
        }		break;
        case 0xe1: {
            // POP HL
            RegHL.reg = memory->readWord(StackPointer.reg);
            std::cout<<"POP HL"<<std::endl;
        }		break;
        case 0xe2: {
            // LD (FF00+C), A
            memory->writeByte(0xFF00 + RegBC.lo, RegAF.hi);
            std::cout<<"LD (FF00+C), A"<<std::endl;
        }		break;
        case 0xe5: {
            // PUSH HL
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, RegHL.reg);
            StackPointer.reg--;
            std::cout<<"PUSH HL"<<std::endl;
        }		break;
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
        }		break;
        case 0xe7: {
            // RST 20h
            uint8_t val = memory->readByte(0xe7);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 20h"<<std::endl;
        }		break;
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
        }		break;
        case 0xe9: {
            // JP HL
            programCounter = RegHL.reg;
            std::cout<<"JP HL"<<std::endl;
        }		break;
        case 0xea: {
            // LD (u16), A
            memory->writeByte(memory->readWord(programCounter), RegAF.hi);
            programCounter += 2;
            std::cout<<"LD (u16), A"<<std::endl;
        }		break;
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
        }		break;
        case 0xef: {
            // RST 28h
            uint8_t val = memory->readByte(0xef);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 28h"<<std::endl;
        }		break;
        case 0xf0: {
            // LD A, (FF00+u8)
            RegAF.hi = memory->readByte(0xFF00 + memory->readByte(programCounter));
            programCounter++;
            std::cout<<"LD A, (FF00+u8)"<<std::endl;
        }		break;
        case 0xf1: {
            // POP AF
            // Flags: ZNHC
            RegAF.reg = memory->readWord(StackPointer.reg);
            // lower nibble of F register must be reset
            RegAF.lo &= 0xF0;
            std::cout<<"POP AF"<<std::endl;
        }		break;
        case 0xf2: {
            // LD A, (FF00+C)
            RegAF.hi = memory->readByte(0xFF00 + RegBC.lo);
            std::cout<<"LD A, (FF00+C)"<<std::endl;
        }		break;
        case 0xf3: {
            // DI 
            std::cout<<"DI "<<std::endl;
        }		break;
        case 0xf5: {
            // PUSH AF
            StackPointer.reg--;
            memory->writeWord(StackPointer.reg, RegAF.reg);
            StackPointer.reg--;
            std::cout<<"PUSH AF"<<std::endl;
        }		break;
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
        }		break;
        case 0xf7: {
            // RST 30h
            uint8_t val = memory->readByte(0xf7);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 30h"<<std::endl;
        }		break;
        case 0xf8: {
            // LD HL, SP+i8
            // Flags: 00HC
            RegHL.reg = StackPointer.reg + (int8_t) memory->readByte(programCounter);
            programCounter++;
            std::cout<<"LD HL, SP+i8"<<std::endl;
        }		break;
        case 0xf9: {
            // LD SP, HL
            StackPointer.reg = RegHL.reg;
            std::cout<<"LD SP, HL"<<std::endl;
        }		break;
        case 0xfa: {
            // LD A, (u16)
            RegAF.hi = memory->readByte(programCounter);
            programCounter += 2;
            std::cout<<"LD A, (u16)"<<std::endl;
        }		break;
        case 0xfb: {
            // EI 
            std::cout<<"EI "<<std::endl;
        }		break;
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
        }		break;
        case 0xff: {
            // RST 38h
            uint8_t val = memory->readByte(0xff);
            StackPointer.reg--;
            memory->writeByte(StackPointer.reg--, programCounter >> 8);
            memory->writeByte(StackPointer.reg, programCounter & 0xff);
            programCounter = 0;
            programCounter |= val;
            std::cout<<"RST 38h"<<std::endl;
        }		break;
        default:
            std::cout << "invalid or unimplemented op code" << std::endl;
            break;
    }
}