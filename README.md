# Game Mandem

Game Mandem is a cycle-accurate Game Boy emulator built in C++. It emulates a cycle accurate version of the Sharp LR35902 CPU, passing all of the individual [Blargg Tests](https://github.com/retrio/gb-test-roms). I really, really, really had fun developing this emulator and hope this is cool or of educational use to anyone looking! <3 

As of now, I have tested the following games to work: **Dr. Mario**, **Tetris**, **Donkey Kong Land**.

## Instructions

As of now, you need to be on a Mac that has SDL2 installed in the library folder for it to run. You can probably tinker around with the makefile pretty easily to get it running on your system, however.

### Setup:

make

./gameboy romFilename debugArg

### Controls:

X = A Button

Z = B Button

Backspace = Select Button

Enter/Return = Start Button

Directional buttons use the directional buttons on the keyboard.

## Screenshots

### Dr. Mario
<img width="657" alt="image" src="https://github.com/Matt-Ng/Game-Mandem/assets/23468554/56b88533-6415-4798-97fe-afa2069a5fe9">
<img width="652" alt="image" src="https://github.com/Matt-Ng/Game-Mandem/assets/23468554/705cfecd-f171-4fdd-a18b-8fc743fcbb95">


### Tetris
<img width="660" alt="image" src="https://github.com/Matt-Ng/Game-Mandem/assets/23468554/73927a6e-1109-4efa-8e4e-7328865b0e32">

<img width="660" alt="image" src="https://github.com/Matt-Ng/Game-Mandem/assets/23468554/a546f6cb-5da7-40ff-88fe-f26d7b2bbafd">



### Donkey Kong Land
![image](https://github.com/Matt-Ng/Game-Mandem/assets/23468554/17c2d982-ff99-4f3b-98fb-a1875479b6f8)

![image](https://github.com/Matt-Ng/Game-Mandem/assets/23468554/82eeeadf-8e2f-4ad2-968b-5141b580d40f)





## Limitations:

Needs to implement sound, currently only MBC0 and MBC1 games can be played.
