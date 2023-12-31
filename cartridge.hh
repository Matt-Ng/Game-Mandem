#include <iostream>
#include <fstream>
#include <string>

#define MAX_GAME_SIZE 0x200000
    
class Cartridge{
    public:
        uint8_t cartridgeMemory[MAX_GAME_SIZE];
        uint8_t fileSize;
        uint8_t currRomBank = 1;
        uint8_t currRamBank = 0;
        uint8_t ramBanks[4][0x2000];

        bool mbc1 = false;
        bool mbc2 = false;

        bool ramBankEnabled = false;
        bool romBankEnabled = false;

        // if false, then it is in simple mode, otherwise advanced
        bool bankMode = false;

        Cartridge(std::string filename);
        void printContents();
        uint8_t readCartridge(uint16_t address);
        uint8_t readRAMBank(uint16_t address);
        void toggleBanking(uint16_t address, uint8_t data);
};