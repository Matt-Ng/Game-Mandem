#include "joypad.hh"

Joypad::Joypad(Memory *memory, SDL_Window *window, SDL_Texture *texture, SDL_Renderer *renderer){
    this->memory = memory;
    this->window = window;
    this->texture = texture;
    this->renderer = renderer;

    memory->writeByte(JOYPAD_REGISTER, 0xFF);
}

void Joypad::releaseKey(int key){
    uint8_t reg = memory->readByte(JOYPAD_REGISTER);
    reg |= (1 << key);
    memory->writeByte(JOYPAD_REGISTER, reg);


}

void Joypad::pressKey(int key, bool isButton){
    uint8_t reg = memory->readByte(JOYPAD_REGISTER);
    reg &= ~(1 << key);

    if(isButton){
        // unset 5th bit to denote that we are in button mode, set 4th to turn off direction mode
        reg &= ~(1 << 5);
        reg |= (1 << 4);
    }
    else{
        reg &= ~(1 << 4);
        reg |= (1 << 5);
    }

    memory->writeByte(JOYPAD_REGISTER, reg);
}

void Joypad::keyPoll(){
    SDL_Event e;

    SDL_PollEvent(&e);

    // TODO: implement joypad interrupt

    switch(e.type){
        case SDL_QUIT:
            SDL_Quit();
            SDL_DestroyTexture(texture);
            SDL_DestroyWindow(window);
            SDL_DestroyRenderer(renderer);
            exit(1);
        case SDL_KEYDOWN:
            switch(e.key.keysym.sym){
                case SDLK_x:
                    pressKey(JOYPAD_A, true);
                    break;
                case SDLK_z:
                    pressKey(JOYPAD_B, true);
                    break;
                case SDLK_BACKSPACE:
                    pressKey(JOYPAD_SELECT, true);
                    break;
                case SDLK_RETURN:
                    pressKey(JOYPAD_START, true);
                    break;
                case SDLK_RIGHT:
                    pressKey(JOYPAD_RIGHT, false);
                    break;
                case SDLK_LEFT:
                    pressKey(JOYPAD_LEFT, false);
                    break;
                case SDLK_UP:
                    pressKey(JOYPAD_UP, false);
                    break;
                case SDLK_DOWN:
                    pressKey(JOYPAD_DOWN, false);
                    break;
            }
            break;
        
        case SDL_KEYUP:
            switch(e.key.keysym.sym){
                case SDLK_x:
                    releaseKey(JOYPAD_A);
                    break;
                case SDLK_z:
                    releaseKey(JOYPAD_B);
                    break;
                case SDLK_BACKSPACE:
                    releaseKey(JOYPAD_SELECT);
                    break;
                case SDLK_RETURN:
                    releaseKey(JOYPAD_START);
                    break;
                case SDLK_RIGHT:
                    releaseKey(JOYPAD_RIGHT);
                    break;
                case SDLK_LEFT:
                    releaseKey(JOYPAD_LEFT);
                    break;
                case SDLK_UP:
                    releaseKey(JOYPAD_UP);
                    break;
                case SDLK_DOWN:
                    releaseKey(JOYPAD_DOWN);
                    break;
            }
            break;
    }
}
