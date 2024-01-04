#include "cartridge.hh"

Cartridge::Cartridge(std::string filename){
    printf("filename... %s\n", filename.c_str());
    std::ifstream GB_ROM(filename, std::ios::binary);
    
    if(GB_ROM.fail()){
        std::cout << "could not open file... :(" << std::endl;
    }

    GB_ROM.seekg(0, std::ios::end);
    fileSize = GB_ROM.tellg();
    GB_ROM.seekg(0, std::ios::beg);

    printf("filesize: %d\n", fileSize);

    GB_ROM.read((char *)cartridgeMemory, fileSize);

    if(cartridgeMemory[0x147] == 1 || cartridgeMemory[0x147] == 2 || cartridgeMemory[0x147] == 3){
        printf("mbc1\n");
        mbc1 = true;
    }
    else if(cartridgeMemory[0x147] == 5 || cartridgeMemory[0x147] == 6){
        printf("mbc2\n");
        mbc2 = true;
    }
    else{
        printf("other mbc\n");
    }

    memset(&ramBanks, 0, sizeof(ramBanks));

    printInfo();
}

void Cartridge::printInfo(){

    // title
    std::string title = "";
    for(int i = 0x134; i <= 0x143; i++){
        title += (char) cartridgeMemory[i];
    }

    // manufacturer code
    std::string manCode = "";
    for(int i = 0x13f; i <= 0x142; i++){
        manCode += (char) cartridgeMemory[i];
    }

    std::cout << "title: " << title << std::endl;
    std::cout << "manufacturer code: " << manCode << std::endl;

}

uint8_t Cartridge::readCartridge(uint16_t address){
    return cartridgeMemory[(currRomBank * 0x4000) + (address - 0x4000)];
}

uint8_t Cartridge::readRAMBank(uint16_t address){
    return ramBanks[currRamBank][address - 0xA000];
}

void Cartridge::toggleBanking(uint16_t address, uint8_t data){
    if(address <= 0x3FFF && (mbc1 || mbc2)){
        // toggle ram bank
        if(mbc2){
            // do nothing if 4th bit is 1
            if(((address >> 4) & 1) == 1){
                return;
            }
        }
        // if lower nibble is 0xA then we enable the ram bank
        // if it is 0x0, disable ram bank
        uint8_t maskedData = data & 0xF;
        if(maskedData == 0xA){
            ramBankEnabled = true;
        }
        else if(maskedData == 0x0){
            ramBankEnabled = false;
        }
    }
    else if(address >= 0x2000 && address <= 0x3FFF && (mbc1 || mbc2)){
        // rom bank change
        if(mbc1){
            // remove lowest 5 bits
            currRomBank &= 0xE0;

            // set to lowest 5 bits of data
            currRomBank |= (data & 0x1F);
        }
        else if(mbc2){
            currRomBank = data & 0xF;
        }

        if(currRomBank == 0){
            currRomBank = 1;
        }
    }
    else if(address >= 0x4000 && address <= 0x5FFF && mbc1){
        // advanced mode ram bank change
        if(bankMode){
            currRamBank = data & 0x03;
        }
        // extended rom bank change
        else{
            // turn off top 3 bits of rombank, turn off bottom 5 bits of data
            currRomBank &= 0x1F;
            data &= 0xE0;

            currRomBank |= data;

            if (currRomBank == 0){
                currRomBank = 1;
            }
        }
    }
    else if(address >= 0x6000 && address <= 0x7FFF && mbc1){
        bankMode = (data & 1) ? true : false;
        // if in simple mode, lock ram bank number to 0
        if(!bankMode){
            currRamBank = 0;
        }
    }

}