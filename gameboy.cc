#include <iostream>

#include "gameboy.hh"

Gameboy::Gameboy(std::string filename){
    cartridge = new Cartridge(filename);
    memory = new Memory(cartridge);
    interrupt = new Interrupt(memory);
    timer = new Timer(memory, interrupt);
    cpu = new CPU(memory, interrupt, timer);
}

void Gameboy::toggleDebugMode(bool val){
    cpu->toggleDebugMode(val);
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

        cpu->handleInterrupts();

        // serial debug
        if (memory->readByte(0xFF02) == 0x81){
            printf("BEEP BEEP BEEP: %c", memory->readByte(0xFF01));
        }
        
    }
}

int main(int argc, char **argv){
    if(argc < 2){
        std::cout << "usage: ./gameboy filename" << std::endl;
    }

    Gameboy *gameboy = new Gameboy(argv[1]);

    if(argc == 3){
        gameboy->toggleDebugMode(true);
    }
    // should take 143 updates to pass the test
    int i = 0;
    int updates = 0;
    while(i < 143){
        gameboy->update();
        updates += MAX_CYCLE;
        i++;
    }
    std::cout<<(updates)<<std::endl;
    
}