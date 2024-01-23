#include <iostream>

#include "ppu.hh"

PPU::PPU(Memory *memory, Interrupt *interrupt){
    this->memory = memory;
    this->interrupt = interrupt;
    this->scanlineCycles = 0;
}

void PPU::step(uint8_t cycles){
    scanlineCycles += cycles;
    //printf("scanline cycles: %d\n", scanlineCycles);
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

    //printf("curr ppumode: %d\n", ppuMode);

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
            status &= ~1;
            status &= ~(1 << 1);
            bool requestInterrupt = status & (1 << 3);

            // should render line here
            renderScanline();
            
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
                bool requestInterrupt = status & (1 << 4);

                if(requestInterrupt){
                    interrupt->requestInterrupt(LCD);
                }
                interrupt->requestInterrupt(VBLANK);
            }
            else{
                // if have not hit the 144 line limit, switch to mode 2
                status &= ~1;
                status |= (1<<1);
                bool requestInterrupt = status & (1 << 5);

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
            //printf("currline: %d\n", getCurrLine());

            // frame is over, reset
            if(getCurrLine() == 154){
                scanlineCycles = 0;
                // switch to mode 2
                status &= ~1;
                status |= (1<<1);

                //printf("new status: %d\n", status & 0x3);

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

void PPU::renderScanline(){
    uint8_t lcdControlRegister = memory->readByte(LCD_CONTROL);
    
    bool spriteDisplayEnable = lcdControlRegister & (1 << 1);
    bool bgDisplayEnable = lcdControlRegister & 1;

    if(bgDisplayEnable){
        // draw background and window tiles
        drawBackground();
    }

    if(spriteDisplayEnable){
        // draw sprite
        drawSprite();
    }
}

void PPU::drawBackground(){
    //printf("drawing...\n");
    uint8_t lcdControlRegister = memory->readByte(LCD_CONTROL);

    // 0 = 0x9800-0x9BFF, 1 = 0x9C00-0x9FFF
    bool windowTileMapSelect = lcdControlRegister & (1 << 6);
    bool windowDisplayEnable = lcdControlRegister & (1 << 5);
    // 0 = 0x8800-0x97FF and the identity number will be signed, 1 = 0x8000-0x8FFF
    bool bgWindowTileSelect = lcdControlRegister & (1 << 4);
    // 0 = 0x9800-0x9bff, 1 = 0x9C00-0x9FFF
    bool bgTileMapSelect = lcdControlRegister & (1 << 3);

    uint8_t scrollX = memory->readByte(SCROLL_X);
    uint8_t scrollY = memory->readByte(SCROLL_Y);
    uint8_t windowX = memory->readByte(WINDOW_X) - 7;
    uint8_t windowY = memory->readByte(WINDOW_Y);

    uint16_t backgroundLoc = 0x9800;
    uint16_t tileDataLoc = 0x8800;

    if(bgTileMapSelect){
        backgroundLoc = 0x9C00; 
    }

    if(bgWindowTileSelect){
        tileDataLoc = 0x8000;
    }

    if(windowTileMapSelect){
        backgroundLoc = 0x9C00;
    }

    uint16_t yTilePixel;
    
    if(windowDisplayEnable && getCurrLine() >= windowY){
        // if we are rendering the window, subtract line by window y
        yTilePixel = getCurrLine() - windowY;
    }
    else{
        // otherwise, 
        yTilePixel = (scrollY + getCurrLine()) & 0xFF;
    }

    uint8_t yTile = (yTilePixel/8) * 32;

    for(int i = 0; i < 160; i++){
        uint8_t xTilePixel = scrollX + i;

        if(windowDisplayEnable && getCurrLine() >= windowY && i >= windowX){
            xTilePixel = i - windowX;
        }

        uint16_t xTile = (xTilePixel/8) & 0x1F;
        uint16_t tileAddress = backgroundLoc + yTile + xTile;
        int tileNumber = memory->readByte(tileAddress);

        uint16_t currTileDataLoc = tileDataLoc;

        // where the signed tile identity comes into place
        if(!bgWindowTileSelect){
            currTileDataLoc += ((tileNumber + 128) * 16);
        }
        else{
            currTileDataLoc += (((uint16_t) tileNumber) * 16);
        }

        /*
            to find the location in memory correlated with the y tile pixel relative to the current tile
            we need to also multiply by 2 due to the fact that each line is 2 bytes long
        */ 
        uint16_t verticalPos = (yTilePixel % 8) * 2;
        uint8_t pixelByteLo = memory->readByte(currTileDataLoc + verticalPos);
        uint8_t pixelByteHi = memory->readByte(currTileDataLoc + verticalPos + 1);

        int horizontalOffset = (7 - (xTilePixel % 8));
        uint8_t colourBitLo = pixelByteLo & (1 << horizontalOffset);
        uint8_t colourBitHi = pixelByteHi & (1 << horizontalOffset);

        if(getCurrLine() >= 0 && getCurrLine() < 144 && xTilePixel >= 0 && xTilePixel < 160){
            lcd[getCurrLine()][xTilePixel] = getColour(colourBitHi, colourBitLo, BGP);
            //printf("colour being pushed: (%d, %d, %d, %d)\n", lcd[getCurrLine()][xTilePixel].r, lcd[getCurrLine()][xTilePixel].g, lcd[getCurrLine()][xTilePixel].b, lcd[getCurrLine()][xTilePixel].a);
        }
    }
}

void PPU::drawSprite(){
    uint8_t lcdControlRegister = memory->readByte(LCD_CONTROL);

    // 0 = 8x8, 1 = 8x16
    bool spriteSize = lcdControlRegister & (1 << 2);

    uint8_t height = 8;

    // there can only be max 10 sprites stored in OAM buffer
    int spritesStored = 0;

    if(spriteSize){
        height = 16;
    }

    for(int i = 0; i < 40; i++){
        if(spritesStored == 10){
            break;
        }
        uint16_t index = 0xFE00 + (i * 4);

        uint8_t ySpritePixel = memory->readByte(index) - 16;
        uint8_t xSpritePixel = memory->readByte(index + 1) - 8;
        uint8_t spriteTileNumber = memory->readByte(index + 2);
        uint8_t spriteAttributes = memory->readByte(index + 3);

        uint8_t spritePriority = spriteAttributes & (1 << 7);
        bool yFlip = spriteAttributes & (1 << 6);
        bool xFlip = spriteAttributes & (1 << 5);
        uint8_t paletteSelect = spriteAttributes & (1 << 4);

        if(xSpritePixel > 0 && getCurrLine() >= ySpritePixel && getCurrLine() < ySpritePixel + height && spritesStored < 10){
            int verticalPos = getCurrLine() - ySpritePixel;
        
            if(yFlip){
                verticalPos = 7 - getCurrLine() - ySpritePixel;
            }
            
            uint16_t spriteAddress = 0x8000 + (spriteTileNumber * 16) + (2 * verticalPos);

            // go through all 8 horizontal pixels of the sprite in this line
            for(int j = 0; j < 8; j++){
                int currBit = j;
                
                if(xFlip){
                    currBit = 7 - j;
                }

                uint8_t pixelByteLo = memory->readByte(spriteAddress);
                uint8_t pixelByteHi = memory->readByte(spriteAddress + 1);

                uint8_t colourBitLo = pixelByteLo & (1 << currBit);
                uint8_t colourBitHi = pixelByteHi & (1 << currBit);

                SDL_Color currColour;
                
                if(paletteSelect){
                    currColour = getColour(colourBitHi, colourBitLo, OBP1);
                }
                else{
                    currColour = getColour(colourBitHi, colourBitLo, OBP0);
                }

                // if white, the pixel is considered transparent
                if(currColour.a != 255 && currColour.g != 255 && currColour.b != 255){
                    uint16_t xPixelPos = xSpritePixel - j + 7;
                    if(getCurrLine() >= 0 && getCurrLine() < 144 && xPixelPos >= 0 && xPixelPos < 160){
                        //printf("colour being pushed: (%d, %d, %d, %d)\n", currColour.r, currColour.g, currColour.b, currColour.a);

                        // either there is no background or window rendered or the sprite has priority
                        if(lcd[getCurrLine()][xPixelPos].a == 0 || spritePriority){
                            lcd[getCurrLine()][xPixelPos] = currColour;
                        }
                    }
                }
            }
            spritesStored++;
        }
    }
}

void PPU::resetScreen(){
    // default colour will be white with alpha of 0

    for(int i = 0; i < 144; i++){
        for(int j = 0; j < 160; j++){
            lcd[i][j] = {255, 255, 255, 0};
        }
    }
}

SDL_Color PPU::getColour(uint8_t pixelHi, uint8_t pixelLo, uint16_t paletteAddress){
    uint8_t combinedColour = (pixelHi << 1) | (pixelLo);
    uint8_t palette = memory->readByte(paletteAddress);
    uint8_t colour = (palette >> (2 * combinedColour)) & 0x3;

    SDL_Color res = {0, 0, 0, 1};
    switch(colour){
        case 0:
            // white
            res = {255, 255, 255, 255};
            break;
        case 1:
            res = {192, 192, 192, 255};
            break;
        case 2:
            res = {96, 96, 96, 255};
            break;
        default:
            // black
            res = {0, 0, 0, 255};
            break;
    }

    return res;
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
    return memory->readByte(LCD_CONTROL) & (1 << 7);
}