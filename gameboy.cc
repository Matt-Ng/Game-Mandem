#include <iostream>

#include "gameboy.hh"

Gameboy::Gameboy(std::string filename){
    cartridge = new Cartridge(filename);
    memory = new Memory(cartridge);
    interrupt = new Interrupt(memory);
    timer = new Timer(memory, interrupt);
    cpu = new CPU(memory, interrupt, timer);
    ppu = new PPU(memory, interrupt);

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow(
        "Game Mandem",
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        640, 
        480, 
        SDL_WINDOW_RESIZABLE
    );
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA4444,
        SDL_TEXTUREACCESS_STREAMING,
        144,
        160
    );

    joypad = new Joypad(memory, window, texture, renderer);
}

void Gameboy::toggleDebugMode(bool val){
    cpu->toggleDebugMode(val);
}

void Gameboy::renderScreen(){
    std::chrono::time_point<std::chrono::system_clock> currTime = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = currTime - lastFrameTime;

    double elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_seconds).count();
    if(elapsedMilliseconds >= 16.7){
        lastFrameTime = std::chrono::system_clock::now();
        std::vector<SDL_Color> pixels;

        for(int i = 0; i < 144; i++){
            for(int j = 0; j < 160; j++){
                //printf("(%d, %d, %d, %d), ", ppu->lcd[i][j].r, ppu->lcd[i][j].g, ppu->lcd[i][j].b, ppu->lcd[i][j].a);


                pixels.push_back(ppu->lcd[i][j]);   
            }
            //printf("\n");
        }
        SDL_UpdateTexture(texture, nullptr, pixels.data(), 160);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer); 
    }
}

void Gameboy::update(){
    // execution loop
    int cyclesThisUpdate = 0;

    while (cyclesThisUpdate < MAX_CYCLE){
        cyclesThisUpdate += cpu->step();

        // update timer registers
        timer->incrementDIV(cyclesThisUpdate);

        if(timer->clockEnabled()){
            timer->incrementTIMA(cyclesThisUpdate);
        }

        ppu->step(cyclesThisUpdate);
        renderScreen();
        joypad->keyPoll();
        cpu->handleInterrupts();
        
    }

    //printf("\n0xA000: %x\n", memory->readByte(0xA000));
}

int main(int argc, char **argv){
    if(argc < 2){
        std::cout << "usage: ./gameboy filename" << std::endl;
    }

    Gameboy *gameboy = new Gameboy(argv[1]);

    if(argc == 3){
        gameboy->toggleDebugMode(true);
    }
    // should take 143 updates to pass the test
    int i = 0;

    while(true){
        gameboy->update();
        i++;
    }
    
}   