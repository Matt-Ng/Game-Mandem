#include <iostream>

#include "gameboy.hh"

Gameboy::Gameboy(std::string filename){
    cartridge = new Cartridge(filename);
    memory = new Memory(cartridge);
    interrupt = new Interrupt(memory);
    timer = new Timer(memory, interrupt);
    cpu = new CPU(memory, interrupt, timer);
}

void Gameboy::update(){
    // execution loop
    int cyclesThisUpdate = 0;

    while (cyclesThisUpdate < MAX_CYCLE){
        cyclesThisUpdate += cpu->step();

        // update timer registers
        if(cyclesThisUpdate % 256 == 0){
            timer->incrementDIV();
        }

        if(cyclesThisUpdate % timer->timeControl() == 0 && timer->clockEnabled()){
            timer->incrementTIMA();
        }

        
        
    }
}