#include <iostream>

#include "gameboy.hh"
#include "interrupt.hh"

Gameboy::Gameboy(std::string filename){
    cartridge = new Cartridge(filename);
    memory = new Memory(cartridge);
    interrupt = new Interrupt(memory);
    cpu = new CPU(memory, interrupt);
    
}

void Gameboy::update(){
    int cyclesThisUpdate = 0;

    while (cyclesThisUpdate < MAX_CYCLE){
        
    }
}