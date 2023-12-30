#include <iostream>

#include "memory.hh"

#define FLAGS RegAF.lo

#define FLAG_Z 7
#define FLAG_N 6
#define FLAG_H 5
#define FLAG_C 4

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

        CPU();

        void initialize();

        void executeOP(uint8_t opCode);
        uint8_t getFlag(uint8_t flag);
        void setFlag(uint8_t flag, uint8_t val);
        
        uint8_t halfCarry8(uint8_t a, uint8_t b);
        uint8_t halfCarry16(uint16_t a, uint16_t b);

    private:
        Memory *memory;
        
};
