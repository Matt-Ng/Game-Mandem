#include <iostream>

#include "timer.hh"

Timer::Timer(Memory *memory, Interrupt *interrupt){
    this->memory = memory;
    this->interrupt = interrupt;
}

void Timer::incrementDIV(int cycles){
    divCount += cycles;

    // overflow
    if(divCount >= 0xFF){
        divCount -= 0xFF;
        memory->memory[DIV]++;
    }
}

void Timer::incrementTIMA(int cycles){
    // handle overflow
    clockCycles += cycles;

    // update according to TAC rate 
    if(clockCycles >= timeControl()){
        clockCycles -= timeControl();
        if(memory->readByte(TIMA) == 0xFF){
            memory->writeByte(TIMA, memory->readByte(TMA));
            interrupt->requestInterrupt(TIMER);
            return;
        }
        else{
            memory->writeByte(TIMA, memory->readByte(TIMA) + 1);
        }
    }
}

int Timer::timeControl(){
    uint8_t clockSelect =  memory->readByte(TAC) & 0x03;
    switch (clockSelect){
        case 0x00:
            return 1024;
            break;
        case 0x01:
            return 16;
            break;
        case 0x10:
            return 64;
            break;
        case 0x11:
            return 256;
            break;
        default:
            break;
    }
    return 256;
}

bool Timer::clockEnabled(){
    return memory->readByte(TAC) & (1 << 2) ? true : false;
}