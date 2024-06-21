#include "mbc.hh"

MBC1::MBC1(uint8_t *rom, bool hasBattery, int numRamBanks, int numRomBanks){
    this->rom = rom;
    this->hasBattery = hasBattery;
    this->numRamBanks = numRamBanks;
    this->numRomBanks = numRomBanks;
    // instantiate external RAM
    if(numRamBanks > 0){
        this->ram = new uint8_t[numRamBanks * 0x2000];
    }
}

uint8_t MBC1::readMemory(uint16_t address){
    if (address < 0x4000){
        return this->rom[address];
    }
    else if (address < 0x8000){
        uint32_t newAddress = (0x4000 * this->currRomBank) + (address - 0x4000);
        return this->rom[newAddress];
    }
    else if(address >= 0xA000 && address < 0xC000){
        if (this->enableRAM){
            if (this->numRamBanks < 4){
                uint16_t newAddress = (address - 0xA000) % (numRamBanks * 0x2000);
                return this->ram[newAddress];
            }
            else{
                if (this->bankMode){
                    uint16_t newAddress = 0x2000 * this->currRamBank + (address - 0xA000);
                    return this->ram[newAddress];
                }
                else{
                    uint16_t newAddress = address - 0xA000;
                    return this->ram[newAddress];
                }
            }
        }
    }
    return 0xFF;
}

void MBC1::writeMemory(uint16_t address, uint8_t data){
    if (address < 0x2000){
        enableRAM = (data & 0xF) == 0xA;
    }
    else if (address < 0x4000){
        // switching rom banks
        uint8_t bankSwitch = data & 0x1F;
        if (bankSwitch == 0){
            bankSwitch = 1;
        }

        currRomBank = bankSwitch;
    }
    else if(address < 0x6000){
        // if in advanced mode, do ram bank change
        if (this->bankMode){
            this->currRamBank = data & 0x3;
            if (this->currRamBank > this->numRamBanks){
                this->currRamBank = this->numRamBanks - 1;
            }
        }
        else{
            // shift data to the left by 3 bits
            uint8_t bitsTransfer = (data & 0x3) << 5;
            this->currRomBank = (this->currRomBank & 0x1F) | bitsTransfer;
            this->adjustRomBankNum();
        }

    }
    else if(address < 0x8000){
        this->bankMode = data & 0x1;
    }
    else if(address >= 0xA000 && address < 0xC000){
        // external RAM write
        if (enableRAM){
            if(this->numRamBanks < 4){
                uint16_t newAddress = (address - 0xA000) % (this->numRamBanks * 0x2000);
                this->ram[newAddress] = data;
            }
            else{
                if(this->bankMode){
                    uint16_t newAddress = 0x2000 * currRamBank + (address - 0xA000);
                    this->ram[newAddress] = data;
                }
                else{
                    uint16_t newAddress = address - 0xA000;
                    this->ram[newAddress] = data;
                }
            }
        }
    }
}

void MBC1::adjustRomBankNum(){
    if(this->currRamBank == 0x00 || this->currRomBank == 0x20 || this->currRomBank == 0x40 || this->currRomBank == 0x60){
        this->currRomBank++;
    }
}