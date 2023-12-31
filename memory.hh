#include <iostream>

#include "cartridge.hh"

class Memory{
    public:
        Memory(Cartridge *cartridge);

        uint8_t memory[0xFFFF];
        
        void writeByte(uint16_t address, uint8_t content);
        void writeWord(uint16_t address, uint16_t content);
        uint8_t readByte(uint16_t address);
        uint16_t readWord(uint16_t address);
    private:
        Cartridge *cartridge;
};