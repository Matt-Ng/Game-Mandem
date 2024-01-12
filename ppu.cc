#pragma once

#include <iostream>

#include "ppu.hh"

PPU::PPU(Memory *memory, Interrupt *interrupt){
    this->memory = memory;
    
    this->scanlineCycles = 0;
}

void PPU::step(uint8_t cycles){
    setStatus();

    if(isLCDEnabled()){
        scanlineCycles += cycles;
    }
    else{
        uint8_t currStatus = getStatus();
        scanlineCycles = 0;
        memory->memory[LY] = 0;

        // setting first 2 bits to 01 for ppu mode
        currStatus |= 0x01;
        currStatus &= ~(1 << 1);
        memory->writeByte(LCD_STATUS, currStatus);
        return;
    }



    if(scanlineCycles >= 456){
        // move to next scanline
        memory->memory[LY]++;
    }
}

void PPU::setStatus(){
    uint8_t oldStatus = getStatus();
    uint8_t status = getStatus();
    uint8_t ppuMode = status & 0x03;

    // change the ppu status bits to the correct mode

    switch (ppuMode){
    case 2: // OAM Mode
        if(scanlineCycles >= 80){
            // switch to mode 3
            scanlineCycles -= 80;
            status |= 1;
            status |= (1<<1);
        }
        break;
    case 3:
        if(scanlineCycles >= 172){
            // switch to mode 0
            scanlineCycles -= 172;
            status &= 0;
            status &= ~(1 << 1);
        }
    
    default:
        break;
    }

    
    // switch to mode 1
    if(getCurrLine() >= 144){
        // new ppu status is now 01
        status |= 1;
        status &= ~(1 << 1);
    }
    else{

        // switch to mode 2
        if(scanlineCycles <= 80){
            status &= 0;
            status |= (1<<1);
        }
        // switch to mode 3
        else if(scanlineCycles <= 252){
            status |= 1;
            status |= (1<<1);
        }
        // switch to mode 0
        else{
            status &= 0;
            status &= ~(1 << 1);
        }
    }

    // if status changed and the constituent request interrupt bit for the mode is set
    // then request an interrupt
    if(oldStatus != status && status & (1 << (status & 0x03))){
        interrupt->requestInterrupt(LCD);
    }

    if(checkCoincidence()){
        status |= (1<<2);
        if(status & (1 << 6)){
            interrupt->requestInterrupt(LCD);
        }
    }
    else{
        status &= ~(1 << 2);
    }

    memory->writeByte(LCD_STATUS, status);
}

uint8_t PPU::getStatus(){
    return memory->readByte(LCD_STATUS);
}

uint8_t PPU::getCurrLine(){
    return memory->readByte(LY);
}

bool PPU::checkCoincidence(){
    return memory->readByte(LY) == memory->readByte(LYC);
}

bool PPU::isLCDEnabled(){
    // lcd enabled status is on the 7th bit
    return memory->readByte(LCD_STATUS) & (1 << 7);
}