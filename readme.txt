As requested, here's the controller test app. Unfortunately I don't have my stuff in a state that I can test on hardware - I spent most of the afternoon just getting my C compiler working again.

However, it appears to work. Hold the fire button for three seconds (which one varies for joystick or keyboard mode) to switch modes. Otherwise, it counts interrupts and displays the bits, as requested. So that I could learn something from it, I also move a sprite left or right. Under emulation I think it's doing the right thing, at least.

rollerTest.rom is the code to run. 

If you want to rebuild it:

- Install SDCC 3.5.0 (not the latest) 64-bit for Windows from https://sourceforge.net/projects/sdcc/files/sdcc-win64/3.5.0/

- In C:\program files\sdcc\bin, replace sdldz80.exe with my version (provided in tursi_tools)

- Use gnuwin32 or cygwin for Make. I have included the gnuwin32 version but not sure if it works standalone. 

- You might need to change paths in the Makefile. I tried to get them all relative except SDCC.

That should be all you need to make changes. Just type make.

Since it's not a bank-switched cartridge, you can probably build without my hacked linker or "makemegacart.exe", but you'll have to change the makefile's link step.

- All the code is in rollerTest.c. 

- crt0.s has the startup code, the vectors, and the raw interrupt handlers. There's some complexity in the spinner handler around my nmi masking code for libti99coleco - mostly wanted to see if it worked. It shouldn't interfere with your testing since the spinner gets priority.

- look at libti99coleco vdp.h if you need different VDP access functions

- if you need to look at the library source code (which I mostly use here only for the VDP access), it's at https://github.com/tursilion/libti99coleco
