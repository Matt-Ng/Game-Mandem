#include "sprite.hh"

Sprite::Sprite(uint8_t xSpritePixel, uint8_t ySpritePixel, uint8_t spriteTileNumber, uint8_t spriteAttributes, uint16_t memoryAddress){
    this->xSpritePixel = xSpritePixel;
    this->ySpritePixel = ySpritePixel;
    this->spriteTileNumber = spriteTileNumber;
    this->spriteAttributes = spriteAttributes;
    this->memoryAddress = memoryAddress;
}

bool Sprite::operator<(const Sprite &other){
    if(xSpritePixel < other.xSpritePixel){
        return false;
    }
    else if(xSpritePixel == other.xSpritePixel){
        if(memoryAddress < other.memoryAddress){
            return false;
        }
    }
    return true;
}