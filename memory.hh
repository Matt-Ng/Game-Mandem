#pragma once

#include <iostream>

#include "cartridge.hh"
#include "joypad.hh"

#define DIV 0xFF04
#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07

#define LY 0xFF44

#define JOYPAD_REGISTER 0xFF00

class Memory{
    public:
        Memory(Cartridge *cartridge, Joypad *joypad);

        uint8_t memory[0xFFFF];
        
        void loadCartridge();
        void handleRomBanking(uint16_t address, uint8_t content);

        void writeByte(uint16_t address, uint8_t content);
        void writeWord(uint16_t address, uint16_t content);
        
        uint8_t readByte(uint16_t address);
        uint16_t readWord(uint16_t address);
    private:
        Cartridge *cartridge;
        Joypad *joypad;
};