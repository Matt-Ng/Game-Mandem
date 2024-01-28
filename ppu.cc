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

    // reset scanline

    // for(int i = 0; i < 160; i++){
    //     lcd[getCurrLine()][i] = {255, 255, 255, 0};
    // }

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
    uint8_t lcdControlRegister = memory->readByte(LCD_CONTROL);

    // 0 = 0x9800-0x9BFF, 1 = 0x9C00-0x9FFF
    bool windowTileMapSelect = lcdControlRegister & (1 << 6);
    // enables the window display
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

    if(windowTileMapSelect || bgTileMapSelect){
        backgroundLoc = 0x9C00;
    }
    if(bgWindowTileSelect){
        // tile identity number will also be unsigned
        tileDataLoc = 0x8000;
    }

    // represents which row of tiles in the background map
    uint16_t yBackgroundLocOffset = 0;

    // we are getting window tiles
    if(windowDisplayEnable && windowY <= getCurrLine()){
        yBackgroundLocOffset += ((getCurrLine() - windowY)/8) * 32;
    }
    else{
        yBackgroundLocOffset += (((getCurrLine() + scrollY) & 0xFF)/8) * 32;
    }

    for(int i = 0; i < 160; i++){
        uint16_t xBackgroundLocOffset = (scrollX + i)/8;

        if(windowDisplayEnable && windowY <= getCurrLine() && windowX <= i){
            xBackgroundLocOffset = (i - windowX)/8;
        }

        uint16_t tileAddress = backgroundLoc + yBackgroundLocOffset + xBackgroundLocOffset;

        // the tile identity number can be signed and unsigned
        int tileIndentifier = bgWindowTileSelect ? (uint8_t) memory->readByte(tileAddress) : (int8_t) memory->readByte(tileAddress);

        uint16_t tileDataAddress = tileDataLoc;
        if(bgWindowTileSelect){
            tileDataAddress += (tileIndentifier * 16);
        }
        else{
            tileDataAddress += (tileIndentifier + 128) * 16;
        }

        // get exact pixel data
        uint16_t pixelY = (((getCurrLine() + scrollY) & 0xFF) % 8) * 2;
        if(windowDisplayEnable && windowY <= getCurrLine()){
            pixelY = ((getCurrLine() - windowY) % 8) * 2;
        }

        uint16_t pixelX = scrollX + i;
        if(windowDisplayEnable && windowY <= getCurrLine() && windowX <= i){
            pixelX = i - windowX;
        }

        uint8_t loByte = memory->readByte(tileDataAddress + pixelY);
        uint8_t hiByte = memory->readByte(tileDataAddress + pixelY + 1);

        int horizontalOffset = 7 - (pixelX%8);
        

        uint8_t colourBitLo = (loByte >> horizontalOffset) & 1;
        uint8_t colourBitHi = (hiByte >> horizontalOffset) & 1;

        if(getCurrLine() >= 0 && getCurrLine() < 144 && i >= 0 && i < 160){
            lcd[getCurrLine()][i] = getColour(colourBitHi, colourBitLo, BGP);
        }
    }
}

void PPU::drawSprite(){
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

    std::vector<uint8_t> sprites;
    for(int i = 0; i < 40; i++){
        // if each sprite is 4 bytes and there can only be 10, we stop at 40
        if(sprites.size() == 40){
            break;
        }
        uint16_t spriteAddress = 0xFE00 + (i * 4);
        
        uint8_t ySpritePixel = memory->readByte(spriteAddress) - 16;
        uint8_t xSpritePixel = memory->readByte(spriteAddress + 1) - 8;
        uint8_t spriteTileNumber = memory->readByte(spriteAddress + 2);
        uint8_t spriteAttributes = memory->readByte(spriteAddress + 3);

        // does the sprite intersect with the scanline?
        if(ySpritePixel <= getCurrLine() && (ySpritePixel + height) > getCurrLine()){
            printf("curr scanline: %d\n", getCurrLine());
            printf("x: %d, y: %d\n", xSpritePixel, ySpritePixel);
            sprites.push_back(spriteAttributes);
            sprites.push_back(spriteTileNumber);
            sprites.push_back(xSpritePixel);
            sprites.push_back(ySpritePixel);
        }
    }

    while(sprites.size() != 0){
        uint8_t ySpritePixel = sprites.back();
        sprites.pop_back();
        uint8_t xSpritePixel = sprites.back();
        sprites.pop_back();
        uint8_t spriteTileNumber = sprites.back();
        sprites.pop_back();
        uint8_t spriteAttributes = sprites.back();
        sprites.pop_back();      

        if(xSpritePixel > 0 && xSpritePixel < 160){
            uint8_t spritePriority = spriteAttributes & (1 << 7);
            bool yFlip = spriteAttributes & (1 << 6);
            bool xFlip = spriteAttributes & (1 << 5);
            uint8_t paletteSelect = spriteAttributes & (1 << 4);

            int verticalPos = getCurrLine() - ySpritePixel;
            if(yFlip){
                verticalPos = height - (getCurrLine() - ySpritePixel);
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

                // if colour is 0, the pixel is considered transparent
                if(colourBitHi != 0 && colourBitLo != 0){
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
                        if(lcd[getCurrLine()][xPixelPos].a == 0 || !spritePriority){
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

    if(paletteAddress == OBP0 || paletteAddress == OBP1){
        printf("pixel Hi: %d, pixel Lo: %d res colour: %d\n", pixelHi, pixelLo, colour);
    }


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
    memory->memory[LY]++;
}

bool PPU::checkCoincidence(){
    return memory->readByte(LY) == memory->readByte(LYC);
}

bool PPU::isLCDEnabled(){
    // lcd enabled status is on the 7th bit
    return memory->readByte(LCD_CONTROL) & (1 << 7);
}