#pragma once

#include <iostream>

#include "ppu.hh"

PPU::PPU(Memory *memory, Interrupt *interrupt){
    this->memory = memory;
    this->scanlineCycles = 0;
}

void PPU::step(uint8_t cycles){
    scanlineCycles += cycles;

    // first check if the lcd is enabled in the first place
    if(!isLCDEnabled()){
        uint8_t currStatus = getStatus();
        scanlineCycles = 0;
        memory->memory[LY] = 0;

        // setting mode to 0
        currStatus &= 0;
        currStatus &= ~(1 << 1);
        memory->writeByte(LCD_STATUS, currStatus);
        return;
    }
    
    setStatus();
}

void PPU::setStatus(){
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
    case 3: // Drawing mode
        if(scanlineCycles >= 172){
            // switch to mode 0
            scanlineCycles -= 172;
            status &= 0;
            status &= ~(1 << 1);
            bool requestInterrupt = status & (1 << (status & 0x03));

            // should render line here
            
            if(requestInterrupt){
                interrupt->requestInterrupt(LCD);
            }
        }
        break;
    case 0: // HBLANK   
        if(scanlineCycles >= 204){
            scanlineCycles -= 204;  
            
            // increment LY location
            incLine();
            // switch to mode 1
            if(getCurrLine() == 144){
                // new ppu status is now 01 (VBLANK)
                status |= 1;
                status &= ~(1 << 1);
                bool requestInterrupt = status & (1 << (status & 0x04));

                if(requestInterrupt){
                    interrupt->requestInterrupt(LCD);
                }
                interrupt->requestInterrupt(VBLANK);
            }
            else{
                // if have not hit the 144 line limit, switch to mode 2
                status &= 0;
                status |= (1<<1);
                bool requestInterrupt = status & (1 << (status & 0x05));

                if(requestInterrupt){
                    interrupt->requestInterrupt(LCD);
                }
            }
        }
        break;
    case 1: // VBLANK
        if(scanlineCycles >= 456){
            scanlineCycles -= 456;
            // go to next scanline
            incLine();

            // frame is over, reset
            if(getCurrLine() == 154){
                scanlineCycles = 0;
                // switch to mode 2
                status &= 0;
                status |= (1<<1);

                // this resets the value regardless
                memory->writeByte(LY, 0);       
            }
        }
        break;
    }

    // if LY and LYC coincide, set coincidence bit and request interrupt if appropriate
    if(checkCoincidence()){
        status |= (1<<2);
        if(status & (1 << 6)){
            interrupt->requestInterrupt(LCD);
        }
    }
    else{
        // reset bit if coincidence not found
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

void PPU::incLine(){
    memory->memory[LY]++;
}

bool PPU::checkCoincidence(){
    return memory->readByte(LY) == memory->readByte(LYC);
}

bool PPU::isLCDEnabled(){
    // lcd enabled status is on the 7th bit
    return memory->readByte(LCD_STATUS) & (1 << 7);
}