#include <iostream>

#include "timer.hh"

Timer::Timer(Memory *memory, Interrupt *interrupt){
    this->memory = memory;
    this->interrupt = interrupt;
}

void Timer::incrementDIV(){
    memory->writeByte(DIV, memory->readByte(DIV) + 1);
}

void Timer::incrementTIMA(){
    // handle overflow
    if(memory->readByte(TIMA) == 0xFF){
        memory->writeByte(TIMA, memory->readByte(TMA));
        interrupt->requestInterrupt(TIMER);
        return;
    }
    memory->writeByte(TIMA, memory->readByte(TIMA) + 1);
}

uint8_t Timer::timeControl(){
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
    return -1;
}

bool Timer::clockEnabled(){
    return (memory->readByte(TAC) >> 2) ? true : false;
}