#include <iostream>

#include "cpu.hh"
#include "cartridge.hh"
#include "memory.hh"

#define MAX_CYCLE 69905

class Gameboy{
    public:
        void update();
        Gameboy(std::string filename);

    private:
        CPU *cpu;
        Cartridge *cartridge;
        Memory *memory;

};