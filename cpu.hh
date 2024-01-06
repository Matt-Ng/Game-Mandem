#pragma once

#include <iostream>

#include "memory.hh"
#include "interrupt.hh"
#include "timer.hh"

#define FLAGS RegAF.lo

#define FLAG_Z 7
#define FLAG_N 6
#define FLAG_H 5
#define FLAG_C 4

#define VBLANK 0
#define LCD 1
#define TIMER 2
#define SERIAL 3
#define JOYPAD 4

class CPU{
    public:
        uint16_t programCounter;

        union Register{
            struct{
                uint8_t lo;
                uint8_t hi;
            };
            uint16_t reg;
        };

        Register RegAF;
        Register RegBC;
        Register RegDE;
        Register RegHL;

        Register StackPointer;

        bool lastInstructionEI = false;

        uint8_t nonprefixTimings[256] = {
            4, 12, 8, 8, 4, 4, 8, 4, 20, 8, 8, 8, 4, 4, 8, 4, 
            4, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4, 
            0, 12, 8, 8, 4, 4, 8, 4, 0, 8, 8, 8, 4, 4, 8, 4, 
            0, 12, 8, 8, 12, 12, 12, 4, 0, 8, 8, 8, 4, 4, 8, 4, 
            4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
            4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
            4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
            8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, 
            4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
            4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
            4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
            4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
            0, 12, 0, 16, 0, 16, 8, 16, 0, 16, 0, 4, 0, 24, 8, 16, 
            0, 12, 0, 0, 0, 16, 8, 16, 0, 16, 0, 0, 0, 0, 8, 16, 
            12, 12, 8, 0, 0, 16, 8, 16, 16, 4, 16, 0, 0, 0, 8, 16, 
            12, 12, 8, 4, 0, 16, 8, 16, 12, 8, 16, 4, 0, 0, 8, 16
        };

        uint8_t prefixTimings[256] = {
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8, 
            8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8, 
            8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8, 
            8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 
            8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8
        };

        CPU(Memory *memory, Interrupt *interrupt, Timer *timer);

        bool debugMode = false;

        void debugPrint(std::string str);
        void toggleDebugMode(bool val);

        uint8_t step();

        uint8_t executeOP(uint8_t opCode);
        uint8_t executePrefixOP(uint8_t opCode);
        uint8_t getFlag(uint8_t flag);
        void setFlag(uint8_t flag, uint8_t val);
        
        uint8_t halfCarry8(uint8_t a, uint8_t b);
        uint8_t halfCarry8(uint8_t a, uint8_t b, uint8_t res);
        uint8_t halfCarry16(uint16_t a, uint16_t b);
        uint8_t halfCarry16(uint16_t a, uint16_t b, uint16_t res);

        void resetFlags();

        void add8(uint8_t &a, uint8_t b);
        void add16(uint16_t &a, uint16_t b);
        void adc(uint8_t &a, uint8_t b);

        void add8Signed(uint8_t &a, int b);
        void add16Signed(uint16_t &a, int b);

        void sub(uint8_t &a, uint8_t b);
        void sbc(uint8_t &a, uint8_t b);

        void inc8(uint8_t &a);
        void dec8(uint8_t &b);

        void cp(uint8_t a, uint8_t b);

        void rl(uint8_t &reg);
        void rlc(uint8_t &reg);
        void rla();
        void rlca();

        void rr(uint8_t &reg);
        void rrc(uint8_t &reg);
        void rra();
        void rrca();
        
        void sla(uint8_t &reg);
        void sra(uint8_t &reg);
        void srl(uint8_t &reg);
        void swap(uint8_t &reg);

        void bit(uint8_t n, uint8_t reg);
        void set(uint8_t n, uint8_t &reg);
        void res(uint8_t n, uint8_t &reg);

        void handleInterrupts();
        void interruptServiceRoutine(uint8_t interruptCode);

        void test();

    private:
        Memory *memory;
        Interrupt *interrupt;
        Timer *timer;
        
};
