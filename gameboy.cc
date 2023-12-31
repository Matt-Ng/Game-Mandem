#include <iostream>

#include "gameboy.hh"

Gameboy::Gameboy(std::string filename){
    cartridge = new Cartridge(filename);
    memory = new Memory(cartridge);
    cpu = new CPU(memory);
    
}

void Gameboy::update(){
    int cyclesThisUpdate = 0;

    while (cyclesThisUpdate < MAX_CYCLE){
        
    }
}