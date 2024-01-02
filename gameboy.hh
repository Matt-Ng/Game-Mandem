#pragma once

#include <iostream>

#include "cpu.hh"
#include "cartridge.hh"
#include "memory.hh"
#include "interrupt.hh"
#include "timer.hh"

#define VBLANK 0
#define LCD 1
#define TIMER 2
#define SERIAL 3
#define JOYPAD 4    

#define CLOCK_SPEED 4194304
#define MAX_CYCLE 69905

class Gameboy{
    public:
        Gameboy(std::string filename);
        void update();
        void toggleDebugMode(bool val);
    private:
        CPU *cpu;
        Cartridge *cartridge;
        Memory *memory;
        Interrupt *interrupt;
        Timer* timer;

};