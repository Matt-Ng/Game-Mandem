#pragma once

#include <iostream>

#include "memory.hh"

#define INTERRUPT_ENABLE 0xFFFF
#define INTERRUPT_FLAG 0xFF0F

#define VBLANK 0
#define LCD 1
#define TIMER 2
#define SERIAL 3
#define JOYPAD 4

// class for the interrupt register

class Interrupt{
    public:
        Interrupt(Memory *memory);
        void toggleIME(bool enable);
        void requestInterrupt(uint8_t interruptCode);
        bool IME = false;
    private:
        Memory *memory;   
};