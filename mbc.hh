#pragma once

#include <iostream>

#define MAX_GAME_SIZE 0x200000

class MBC{
    public:
        uint8_t *rom;
        uint8_t *ram;
        bool hasBattery;
        uint8_t currRomBank = 1;
        uint8_t currRamBank = 0;
        uint8_t numRomBanks;
        uint8_t numRamBanks;
        virtual uint8_t readMemory(uint16_t){return 0;};
        virtual void writeMemory(uint16_t, uint8_t){};
};

class MBC0 : public MBC {
    public:
        MBC0(uint8_t *rom){
            this->rom = rom;
        };
        uint8_t readMemory(uint16_t address) override {
            return rom[address];
        };
};

class MBC1 : public MBC {
    public:
        uint16_t lowBankNumber = 0;
        uint16_t highBankNumber = 0;
        bool enableRAM = false;
        MBC1(uint8_t *rom, bool hasBattery, int numRamBanks, int numRomBanks);
        // 0 = simple, 1 = advanced
        bool bankMode = false;
        uint8_t readMemory(uint16_t address) override;
        void writeMemory(uint16_t address, uint8_t data) override;
        void adjustRomBankNum();
};