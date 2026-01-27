# TI-8 a CHIP8 for your TI-84 Plus CE Calculator   

### Almost Complete Chip8 emulator!! 
This is a CHIP8 emulator that runs on a TI-84 Plus CE Calulator.
---
It requires the the clib.8xg (libc for this calculator)to run. [Here is the link](https://github.com/CE-Programming/libraries/releases)   
It currently works pretty alright there still needs to be some tweaks here and there and a improved UI. You can quit the emulator loop by pressing the mode key to return back to the rom selection menu.   
The rom selction menu prompts you to select a rom. It expects you to inter a valid number between 0 and however many roms you embed. To view all roms just type 'A' and press enter it will display all roms that you embeded. To exit the menu press 'Q'
All roms must be embeded into the program, this can be done by using the python script included with the program. It generates a header file with all off the binary data is then compiled into your program.    

The Keyboard Layout
---
![Keyboard Layout](/emu-layout.jpg)
