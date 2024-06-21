#include "cartridge.hh"

Cartridge::Cartridge(std::string filename){
    //printf("filename... %s\n", filename.c_str());
    std::ifstream GB_ROM(filename, std::ios::binary);
    
    if(GB_ROM.fail()){
        std::cout << "could not open file... :(" << std::endl;
    }

    GB_ROM.seekg(0, std::ios::end);
    this->fileSize = GB_ROM.tellg();
    GB_ROM.seekg(0, std::ios::beg);

    //printf("filesize: %d\n", fileSize);

    GB_ROM.read((char *)this->rom, fileSize);
    // memset(&ramBanks, 0, sizeof(ramBanks));
    this->getRomBanks();
    this->getRamBanks();
    this->getMBC();

    printInfo();
}

void Cartridge::printInfo(){
    // title
    std::string title = "";
    for(int i = 0x134; i <= 0x143; i++){
        title += (char) this->rom[i];
    }

    // manufacturer code
    std::string manCode = "";
    for(int i = 0x13f; i <= 0x142; i++){
        manCode += (char) this->rom[i];
    }

    std::cout << "title: " << title << std::endl;
    std::cout << "manufacturer code: " << manCode << std::endl;
    std::cout << "mbc type: " << this->mbcNum << std::endl;
}

uint8_t Cartridge::readCartridge(uint16_t address){
    return this->mbc->readMemory(address);
}

void Cartridge::writeCartridge(uint16_t address, uint8_t data){
    this->mbc->writeMemory(address, data);
}

void Cartridge::getMBC(){
    uint8_t mbcType = this->rom[MBC_ADDRESS];
    std::cout << "mbc type: " << (int) mbcType << std::endl;
    switch (mbcType)
    {
    case 0x00:
        this->mbc = new MBC0(this->rom);
        this->mbcNum = 0;
        break;
    case 0x01:
        std::cout << "CREATING DAT MBC1"   << std::endl;
        this->mbc = new MBC1(this->rom, false, this->numRamBanks, this->numRomBanks);
        this->mbcNum = 1;
        break;
    case 0x02:
        this->mbc = new MBC1(this->rom, false, this->numRamBanks, this->numRomBanks);
        this->mbcNum = 1;
        break;
    case 0x03:
        this->mbc = new MBC1(this->rom, true, this->numRamBanks, this->numRomBanks);
        this->mbcNum = 1;
        break;
    default:
        break;
    }
}

void Cartridge::getRomBanks(){
    uint8_t romType = this->rom[ROM_SIZE_ADDRESS];

    this->numRomBanks = 2 << romType;
}

void Cartridge::getRamBanks(){
    uint8_t ramType = this->rom[RAM_SIZE_ADDRESS];
    switch(ramType){
        case 0x2:
            this->numRamBanks = 1;
            break;
        case 0x3:
            this->numRamBanks = 4;
            break;
        case 0x4:
            this->numRamBanks = 16;
            break;
        case 0x5:
            this->numRamBanks = 8;
            break;
        default:
            this->numRamBanks = 0;
            break;
    }
}