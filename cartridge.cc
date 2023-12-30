#include <iostream>
#include <fstream>

int main(){
    std::ifstream GB_ROM("DMG_ROM.bin", std::ios::binary);
    GB_ROM.seekg(0, std::ios::end);
    int fileSize = GB_ROM.tellg();
    GB_ROM.seekg(0, std::ios::beg);

    uint8_t cartridgeFile[fileSize];
    GB_ROM.read((char *)cartridgeFile, fileSize);

    for(int i = 0; i < fileSize; i++){
        printf("%x\n", cartridgeFile[i]);
    }
    printf("filesize: %d\n", fileSize);
    return 1;
}