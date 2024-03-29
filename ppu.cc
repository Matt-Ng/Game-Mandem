#include <iostream>

#include "ppu.hh"

PPU::PPU(Memory *memory, Interrupt *interrupt){
    this->memory = memory;
    this->interrupt = interrupt;
    this->scanlineCycles = 0;

    resetScreen();
}

void PPU::step(uint16_t cycles){
    scanlineCycles += cycles;
    // first check if the lcd is enabled in the first place
    
    if(!isLCDEnabled()){
        uint8_t currStatus = getStatus();
        scanlineCycles = 0;
        memory->memory[LY] = 0;
        internalWindowLine = 0;
        windowInLine = 0;

        // setting mode to 0
        currStatus &= ~1;
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

            // switch to mode 1
            if(getCurrLine() == 144){
                // new ppu status is now 01 (VBLANK)
                status |= 1;
                status &= ~(1 << 1);
                bool requestInterrupt = status & (1 << 4);

                drawLCD = true;
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

            // frame is over, reset
            if(getCurrLine() == 154){
                internalWindowLine = 0;
                windowInLine = false;

                scanlineCycles = 0;
                // switch to mode 2
                status &= ~1;
                status |= (1<<1);

                // this resets the value regardless
                memory->writeByte(LY, 0);
            }
        }
        break;
    }

    memory->writeByte(LCD_STATUS, status);
}

void PPU::renderScanline(){
    uint8_t lcdControlRegister = memory->readByte(LCD_CONTROL);
    
    bool spriteDisplayEnable = lcdControlRegister & (1 << 1);
    bool bgDisplayEnable = lcdControlRegister & 1;
    bool windowDisplayEnable = lcdControlRegister & (1 << 5);

    // reset scanline

    // for(int i = 0; i < 160; i++){
    //     lcd[getCurrLine()][i] = {255, 255, 255, 0};
    // }

    // we want to keep track of colour values from the background so that we can display sprites accurately
    int scanLine[160] = {0};

    if(bgDisplayEnable){
        // draw background and window tiles
        drawBackground(scanLine);
    }

    if(windowDisplayEnable){
        drawWindow(scanLine);
    }
    else{
        // for the internal window line counter
        windowInLine = false;
    }

    if(spriteDisplayEnable){
        // draw sprite
        drawSprite(scanLine);
    }
}

void PPU::drawBackground(int *scanLine){
    uint8_t lcdControlRegister = memory->readByte(LCD_CONTROL);

    // 0 = 0x8800-0x97FF and the identity number will be signed, 1 = 0x8000-0x8FFF
    bool bgWindowTileSelect = lcdControlRegister & (1 << 4);
    // 0 = 0x9800-0x9bff, 1 = 0x9C00-0x9FFF
    bool bgTileMapSelect = lcdControlRegister & (1 << 3);

    uint8_t scrollX = memory->readByte(SCROLL_X);
    uint8_t scrollY = memory->readByte(SCROLL_Y);

    uint16_t backgroundLoc = 0x9800;
    uint16_t tileDataLoc = 0x8800;

    if(bgTileMapSelect){
        backgroundLoc = 0x9C00;
    }

    if(bgWindowTileSelect){
        // tile identity number will also be unsigned
        tileDataLoc = 0x8000;
    }

    // represents which row of tiles in the background map
    uint16_t yBackgroundLocOffset = (((getCurrLine() + scrollY) % 256)/8) * 32;

    for(int i = 0; i < 160; i++){
        uint8_t xBackgroundLocOffset = ((scrollX + i) & 0xFF)/8;

        uint16_t tileAddress = backgroundLoc + yBackgroundLocOffset + xBackgroundLocOffset;
        
        uint16_t tileDataAddress = tileDataLoc;
        if(bgWindowTileSelect){
            uint8_t tileIndentifier = memory->readByte(tileAddress);
            tileDataAddress += (tileIndentifier * 16);
        }
        else{
            int8_t tileIndentifier = (int8_t) memory->readByte(tileAddress);
            tileDataAddress += (tileIndentifier + 128) * 16;
        }

        // get exact pixel data
        uint16_t pixelY = (((getCurrLine() + scrollY)) % 8) * 2;

        uint16_t pixelX = (scrollX + i) & 0xFF;

        uint8_t loByte = memory->readByte(tileDataAddress + pixelY);
        uint8_t hiByte = memory->readByte(tileDataAddress + pixelY + 1);

        int horizontalOffset = 7 - (pixelX%8);

        uint8_t colourBitLo = (loByte >> horizontalOffset) & 1;
        uint8_t colourBitHi = (hiByte >> horizontalOffset) & 1;

        if(getCurrLine() >= 0 && getCurrLine() < 144 && i >= 0 && i < 160){
            lcd[getCurrLine()][i] = getColour(colourBitHi, colourBitLo, BGP);

            // if((getCurrLine() + scrollY)%8 == 7){
            //     lcd[getCurrLine()][i] = {0,67,0,255};
            // }

            // if(getCurrLine() == 127){
            //     lcd[getCurrLine()][i] = {255, 99, 71, 255};
            // }
            // if(getCurrLine() == 128){
            //     lcd[getCurrLine()][0] = {67, 255, 71, 255};
            // }
            // if(getCurrLine() == 129){
            //     lcd[getCurrLine()][0] = {67, 90, 255, 255};
            // }            
            // we want to get the colour value before the palette for sprite display 
            scanLine[i] = (colourBitHi << 1) | (colourBitLo);
        }
    }
}

void PPU::drawWindow(int *scanLine){
    uint8_t lcdControlRegister = memory->readByte(LCD_CONTROL);

    // 0 = 0x9800-0x9BFF, 1 = 0x9C00-0x9FFF
    bool windowTileMapSelect = lcdControlRegister & (1 << 6);
    // 0 = 0x8800-0x97FF and the identity number will be signed, 1 = 0x8000-0x8FFF
    bool bgWindowTileSelect = lcdControlRegister & (1 << 4);

    uint8_t windowX = memory->readByte(WINDOW_X);
    uint8_t windowY = memory->readByte(WINDOW_Y);

    // do not fetch window if we haven't reached window y value yet
    if(getCurrLine() < windowY){
        return;
    }

    if(windowX >= 7 && windowX < 166 && windowY >= 0 && windowY < 144){
        windowInLine = true;
    }
    else{
        windowInLine = false;
        return;
    }

    windowX -= 7;

    uint16_t backgroundLoc = 0x9800;
    uint16_t tileDataLoc = 0x8800;

    if(windowTileMapSelect){
        backgroundLoc = 0x9C00;
    }

    if(bgWindowTileSelect){
        // tile identity number will also be unsigned
        tileDataLoc = 0x8000;
    }

    // represents which row of tiles in the background map
    uint16_t yBackgroundLocOffset = (internalWindowLine/8) * 32;

    for(int i = 0; i < 160; i++){
        if(i < windowX){
            continue;
        }

        uint16_t xBackgroundLocOffset = (i - windowX)/8;

        uint16_t tileAddress = backgroundLoc + yBackgroundLocOffset + xBackgroundLocOffset;

        int tileIndentifier = bgWindowTileSelect ? (uint8_t) memory->readByte(tileAddress) : (int8_t) memory->readByte(tileAddress);

        uint16_t tileDataAddress = tileDataLoc;
        if(bgWindowTileSelect){
            tileDataAddress += (tileIndentifier * 16);
        }
        else{
            tileDataAddress += (tileIndentifier + 128) * 16;
        }

        // get exact pixel data
        uint16_t pixelY = ((internalWindowLine) % 8) * 2;

        uint16_t pixelX = i - windowX;

        uint8_t loByte = memory->readByte(tileDataAddress + pixelY);
        uint8_t hiByte = memory->readByte(tileDataAddress + pixelY + 1);

        int horizontalOffset = 7 - (pixelX%8);

        uint8_t colourBitLo = (loByte >> horizontalOffset) &1;
        uint8_t colourBitHi = (hiByte >> horizontalOffset) & 1;

        if(getCurrLine() >= 0 && getCurrLine() < 144 && i >= 0 && i < 160 && windowX <= i){
            lcd[getCurrLine()][i] = getColour(colourBitHi, colourBitLo, BGP);
            scanLine[i] = (colourBitHi << 1) | (colourBitLo);
            windowInLine = true;
        }
    }
}

void PPU::drawSprite(int *scanLine){
    uint8_t lcdControlRegister = memory->readByte(LCD_CONTROL);

    // 0 = 8x8, 1 = 8x16
    bool spriteSize = lcdControlRegister & (1 << 2);
 
    uint8_t height = 8;

    if(spriteSize){
        height = 16;
    }

    /**
     * each sprite has 4 bytes. we want to push them in backwards order
     * so that we can pop each component efficiently in a stack manner
     */

    std::vector<Sprite *> sprites;
    for(int i = 0; i < 40; i++){
        // if each sprite is 4 bytes and there can only be 10, we stop at 40
        if(sprites.size() == 10){
            break;
        }
        uint16_t spriteAddress = 0xFE00 + (i * 4);
        
        uint8_t ySpritePixel = memory->readByte(spriteAddress) - 16;
        uint8_t xSpritePixel = memory->readByte(spriteAddress + 1) - 8;
        uint8_t spriteTileNumber = memory->readByte(spriteAddress + 2);
        uint8_t spriteAttributes = memory->readByte(spriteAddress + 3);

        Sprite *currSprite = new Sprite(xSpritePixel, ySpritePixel, spriteTileNumber, spriteAttributes, spriteAddress);

        // does the sprite intersect with the scanline?
        if(ySpritePixel <= getCurrLine() && (ySpritePixel + height) > getCurrLine()){
            sprites.push_back(currSprite);
        }
    }

    std::sort(sprites.begin(), sprites.end());

    while(sprites.size() != 0){
        Sprite *currSprite = sprites.back();
        sprites.pop_back();

        uint8_t ySpritePixel = currSprite->ySpritePixel;
        uint8_t xSpritePixel = currSprite->xSpritePixel;
        uint8_t spriteTileNumber = currSprite->spriteTileNumber;
        uint8_t spriteAttributes = currSprite->spriteAttributes;

        if(height == 16){
            spriteTileNumber &= 0xFE;
        }

        if(xSpritePixel > 0 && xSpritePixel < 159){
            uint8_t spritePriority = spriteAttributes & (1 << 7);
            bool yFlip = spriteAttributes & (1 << 6);
            bool xFlip = spriteAttributes & (1 << 5);
            uint8_t paletteSelect = spriteAttributes & (1 << 4);

            int verticalPos = getCurrLine() - ySpritePixel;
            if(yFlip){
                verticalPos = height - (getCurrLine() - ySpritePixel) - 1;
            }

            uint16_t spriteTileAddress = 0x8000 + (spriteTileNumber * 16) + (2 * verticalPos);

            uint8_t spriteBitLo = memory->readByte(spriteTileAddress);
            uint8_t spriteBitHi = memory->readByte(spriteTileAddress + 1);

            // go through the 8 horizontal pixels of the tile
            for(int i = 0; i < 8; i++){
                uint8_t horizontalPos = i;
                if(xFlip){
                    horizontalPos = 7 - i;
                }

                uint8_t colourBitLo = (spriteBitLo >> horizontalPos) & 1;
                uint8_t colourBitHi = (spriteBitHi >> horizontalPos) & 1;

                // tile pixels are organized in reverse order
                uint16_t xPixelPos = 7 - i + xSpritePixel;
                if(getCurrLine() >= 0 && getCurrLine() < 144 && xPixelPos >= 0 && xPixelPos < 160){
                    //printf("colour being pushed: (%d, %d, %d, %d)\n", currColour.r, currColour.g, currColour.b, currColour.a);
                    // either there is no background or window rendered or the sprite has priority
                    SDL_Colour currColour;
                    if(paletteSelect){
                        currColour = getColour(colourBitHi, colourBitLo, OBP1);
                    }
                    else{
                        currColour = getColour(colourBitHi, colourBitLo, OBP0);
                    }

                    // white pixel = transparent
                    if ((colourBitHi << 1) | (colourBitLo)){
                        // keep in mind that spritePriority = 0 means that sprite is prioritized
                        if((scanLine[xPixelPos] == 0) || !spritePriority){
                            lcd[getCurrLine()][xPixelPos] = currColour;
                        }
                    }
                    
                }
            }
        }
    }
}

void PPU::resetScreen(){
    // default colour will be white

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

    SDL_Color res = {0, 0, 0, 255};
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
    if (windowInLine && getCurrLine() >= memory->readByte(WINDOW_Y) && getCurrLine() < memory->readByte(WINDOW_Y) + 144){
        internalWindowLine++;
    }

    memory->memory[LY]++;
}

bool PPU::checkCoincidence(){
    return memory->readByte(LY) == memory->readByte(LYC);
}

bool PPU::isLCDEnabled(){
    // lcd enabled status is on the 7th bit
    return memory->readByte(LCD_CONTROL) & (1 << 7);
}