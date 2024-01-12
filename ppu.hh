#pragma once

#include <iostream>

#include "memory.hh"
#include "interrupt.hh"

#define LCD 1

#define LY 0xFF44
#define LYC 0xFF45
#define LCD_STATUS 0xFF41

class PPU{
    public:
        uint8_t lcd[160][144][3];
        int scanlineCycles;

        PPU(Memory *memory, Interrupt *interrupt);
        void step(uint8_t cycles);
        void setStatus();

        void vblank();
        uint8_t getCurrLine();
        uint8_t getStatus();

        bool checkCoincidence();
        bool isLCDEnabled();

        

    private:
        Memory *memory;
        Interrupt *interrupt;
};
