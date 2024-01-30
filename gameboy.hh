#pragma once

#include <iostream>
#include <SDL2/SDL.h>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>

#include "cpu.hh"
#include "cartridge.hh"
#include "memory.hh"
#include "interrupt.hh"
#include "timer.hh"
#include "ppu.hh"
#include "joypad.hh"

#define VBLANK 0
#define LCD 1
#define TIMER 2
#define SERIAL 3
#define JOYPAD 4    

#define CLOCK_SPEED 4194304
#define MAX_CYCLE 69905
#define FRAME_TIME 

class Gameboy{
    public:
        std::chrono::time_point<std::chrono::system_clock> lastFrameTime = std::chrono::system_clock::now();
        int frame = 0;

        Gameboy(std::string filename);
        void renderScreen();
        void update();
        void toggleDebugMode(bool val);
    private:
        CPU *cpu;
        Cartridge *cartridge;
        Memory *memory;
        Interrupt *interrupt;
        Timer *timer;
        PPU *ppu;
        Joypad *joypad;

        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *texture;

};