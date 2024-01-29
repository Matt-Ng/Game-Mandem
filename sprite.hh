#pragma once

#include <iostream>

class Sprite{
    public:
        uint8_t xSpritePixel;
        uint8_t ySpritePixel;
        uint8_t spriteTileNumber;
        uint8_t spriteAttributes;
        uint16_t memoryAddress;

        Sprite(uint8_t xSpritePixel, uint8_t ySpritePixel, uint8_t spriteTileNumber, uint8_t spriteAttributes, uint16_t memoryAddress);
        bool operator<(const Sprite &other);

};