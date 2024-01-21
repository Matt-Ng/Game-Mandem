#include <iostream>
#include <SDL2/SDL.h>

#include "memory.hh"

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
        Joypad(Memory *memory);
        void releaseKey(int key);
        void pressKey(int key, bool isButton);
        void keyPoll();

    private:
        Memory *memory;
};