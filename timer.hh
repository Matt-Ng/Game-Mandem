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
        Timer(Memory *memory, Interrupt *interrupt);
        void incrementDIV();
        void incrementTIMA();
        int timeControl();

        bool clockEnabled();
    private:
        Memory *memory;
        Interrupt *interrupt;
};