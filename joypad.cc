#pragma once

#include "joypad.hh"

Joypad::Joypad(SDL_Window *window, SDL_Texture *texture, SDL_Renderer *renderer){
    this->window = window;
    this->texture = texture;
    this->renderer = renderer;
}

uint8_t Joypad::getJoypad(uint8_t currState){
    currState |= 0xF;

    // buttons
    if(!(currState & (1 << 5))){
        if(aButton){
            currState &= ~(1 << JOYPAD_A);
        }
        if(bButton){
            currState &= ~(1 << JOYPAD_B);
        }
        if(selectButton){
            currState &= ~(1 << JOYPAD_SELECT);
        }
        if(startButton){
            currState &= ~(1 << JOYPAD_START);
        }
    }
    
    // direction
    if(!(currState & (1 << 4))){
        if(upButton){
            currState &= ~(1 << JOYPAD_UP);
        }
        if(leftButton){
            currState &= ~(1 << JOYPAD_LEFT);
        }
        if(rightButton){
            currState &= ~(1 << JOYPAD_RIGHT);
        }
        if(downButton){
            currState &= ~(1 << JOYPAD_DOWN);
        }
    }
    return currState;
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
                    aButton = true;
                    break;
                case SDLK_z:
                    bButton = true;
                    break;
                case SDLK_BACKSPACE:
                    selectButton = true;
                    break;
                case SDLK_RETURN:
                    startButton = true;
                    break;
                case SDLK_RIGHT:
                    rightButton = true;
                    break;
                case SDLK_LEFT:
                    leftButton = true;
                    break;
                case SDLK_UP:
                    upButton = true;
                    break;
                case SDLK_DOWN:
                    downButton = true;
                    break;
            }
            break;
        
        case SDL_KEYUP:
            switch(e.key.keysym.sym){
                case SDLK_x:
                    aButton = false;
                    break;
                case SDLK_z:
                    bButton = false;
                    break;
                case SDLK_BACKSPACE:
                    selectButton = false;
                    break;
                case SDLK_RETURN:
                    startButton = false;
                    break;
                case SDLK_RIGHT:
                    rightButton = false;
                    break;
                case SDLK_LEFT:
                    leftButton = false;
                    break;
                case SDLK_UP:
                    upButton = false;
                    break;
                case SDLK_DOWN:
                    downButton = false;
                    break;
            }
            break;
    }
}
