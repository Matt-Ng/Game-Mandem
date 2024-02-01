#pragma once

#include <iostream>
#include <SDL2/SDL.h>

#define JOYPAD_REGISTER 0xFF00

#define JOYPAD_A 0
#define JOYPAD_B 1
#define JOYPAD_SELECT 2
#define JOYPAD_START 3
#define JOYPAD_RIGHT 0
#define JOYPAD_LEFT 1
#define JOYPAD_UP 2
#define JOYPAD_DOWN 3



class Joypad{
    public:
        /**
         * Controls:
         * x = A
         * z = B
         * enter/return = start
         * backspace = select
         * arrow pad = directional controls
         */
        Joypad(SDL_Window *window, SDL_Texture *texture, SDL_Renderer *renderer);

        uint8_t getJoypad(uint8_t currState);
        void keyPoll();

    private:
        bool aButton = false;
        bool bButton = false;
        bool selectButton = false;
        bool startButton = false;

        bool upButton = false;
        bool leftButton = false;
        bool rightButton = false;
        bool downButton = false;

        SDL_Window *window; 
        SDL_Texture *texture; 
        SDL_Renderer *renderer;
};