#pragma once

#include <iostream>
#include <SDL2/SDL.h>

#include "memory.hh"
#include "interrupt.hh"

#define LCD 1

#define LY  0xFF44
#define LYC 0xFF45
#define LCD_STATUS 0xFF41
#define LCD_CONTROL 0xFF40
#define SCROLL_Y 0xFF42
#define SCROLL_X 0xFF43
#define BGP 0xFF47
#define OBP0 0xFF48
#define OBP1 0xFF49
#define WINDOW_Y 0xFF4A
#define WINDOW_X 0xFF4B

class PPU{
    public:
        SDL_Color lcd[144][160];
        int scanlineCycles = 0;

        PPU(Memory *memory, Interrupt *interrupt);
        void step(uint8_t cycles);
        void setStatus();

        void renderScanline();
        void drawBackground();
        void drawSprite();
        void resetScreen();

        void incLine();
        uint8_t getCurrLine();
        uint8_t getStatus();

        bool checkCoincidence();
        bool isLCDEnabled();

        SDL_Color getColour(uint8_t pixelHi, uint8_t pixelLo, uint16_t paletteAddress);
        

    private:
        Memory *memory;
        Interrupt *interrupt;
};
