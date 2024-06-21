#pragma once

#include <iostream>
#include <fstream>
#include <string>

#include "mbc.hh"

#define MAX_GAME_SIZE 0x200000

#define MBC_ADDRESS 0x147
#define ROM_SIZE_ADDRESS 0x148
#define RAM_SIZE_ADDRESS 0x149
    
class Cartridge{
    public:
        int fileSize;
        int mbcNum = 0;
        int numRamBanks;
        int numRomBanks;

        bool ramBankEnabled = false;
        bool romBankEnabled = false;
        uint8_t rom[MAX_GAME_SIZE];

        bool hasBattery;
        bool hasSave;

        // if false, then it is in simple mode, otherwise advanced
        bool bankMode = false;

        Cartridge(std::string filename);
        void printInfo();
        uint8_t readCartridge(uint16_t address);
        void writeCartridge(uint16_t address, uint8_t data);
    
    private:
        MBC *mbc;
        void getMBC();
        void getRomBanks();
        void getRamBanks();
};