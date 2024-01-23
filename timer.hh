#pragma once

#include <iostream>

#include "memory.hh"
#include "interrupt.hh"

#define DIV 0xFF04
#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07

class Timer {
    public:
        int divCount = 0;
        int clockCycles = 0;

        Timer(Memory *memory, Interrupt *interrupt);
        void incrementDIV(int cycles);
        void incrementTIMA(int cycles);
        int timeControl();

        bool clockEnabled();
    private:
        Memory *memory;
        Interrupt *interrupt;
};