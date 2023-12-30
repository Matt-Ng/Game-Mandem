#include <iostream>

class Memory{
    public:
        u_int8_t memory[0xFFFF];
        
        void writeByte(u_int16_t address, u_int8_t content);
        void writeWord(u_int16_t address, u_int16_t content);
        u_int8_t readByte(u_int16_t address);
        u_int16_t readWord(u_int16_t address);
};