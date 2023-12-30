#include <iostream>

#include "memory.hh"

void Memory::writeByte(u_int16_t address, u_int8_t content){
    
    // if read only memory, restrict
    if (address < 0x8000){
        std::cout << "error: cannot write to read only memory" << std::endl;
        return;
    }
    memory[address] = content;
}

void Memory::writeWord(u_int16_t address, u_int16_t content){
    
    // if read only memory, restrict
    if (address < 0x8000){
        std::cout << "error: cannot write to read only memory" << std::endl;
        return;
    }
    memory[address] = content;
}

u_int8_t Memory::readByte(u_int16_t address){
    return memory[address];
}
u_int16_t Memory::readWord(u_int16_t address){
    return memory[address];
}