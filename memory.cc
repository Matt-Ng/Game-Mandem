#include <iostream>

#include "memory.hh"

Memory::Memory(Cartridge *cartridge){
    this->cartridge = cartridge;

    loadCartridge();
    // timer registers
    memory[TIMA] = 0x0;
    memory[TMA] = 0x0;
    memory[TAC] = 0x0; 
}

void Memory::loadCartridge(){
    for(int i = 0; i < 0x4000; i++){
        memory[i] = cartridge->cartridgeMemory[i];
    }
}

void Memory::writeByte(uint16_t address, uint8_t content){


    // serial blargg debug
    if (address == 0xFF02 && content == 0x81){
        printf("%c", readByte(0xFF01));
    }

    // blargg debug
    if (address == 0xA000){
        uint16_t i = 0xA000;
        printf("testing\n");
        while(i < 0xA000 + 256 && memory[i] != '\0'){
            printf("%c\n", memory[i]);
            i++;
        }
    }
    
    // this address range is to handle memory manking in cartridge
    if(address < 0x8000){
        cartridge->toggleBanking(address, content);
        return;
    }
    else if(address >= 0xC000 && address <= 0xDDFF){
        // send to echo ram as well
        memory[address] = content;
        memory[address + 0x2000] = content;
        return;
    }
    else if(address >= 0xFEA0 && address <= 0xFEFF){
        std::cout << "prohibited memory access" << std::endl;
        return;
    }

    memory[address] = content;
}

void Memory::writeWord(uint16_t address, uint16_t content){
    writeByte(address, (uint8_t) (content & 0xFF));
    writeByte(address + 1, (uint8_t) (content >> 8));
}

u_int8_t Memory::readByte(uint16_t address){
    if (address >= 0x4000 && address <= 0x7FFF){
        return cartridge->readCartridge(address);
    }

    if (address >= 0xA000 && address <= 0xBFFF){
        return cartridge->readRAMBank(address);
    }

    return memory[address];
}

u_int16_t Memory::readWord(uint16_t address){
    return ((uint16_t) readByte(address)) | ((uint16_t) readByte(address + 1) << 8);
}
