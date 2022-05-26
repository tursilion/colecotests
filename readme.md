20210509

Various hardware tests for identifying ColecoVision behaviour (not necessarily for verifying hardware is working correctly!)

BIOSMEM: 
- Phoenix boot mode test. First checks for 24k of ROM at >0000, then 8k of RAM at >6000. Then it verifies the 512k RAM exists (partial test), and checks that the memory correctly handles Megacart style banking. It follows up by verifying that the mask register works to limit 256k, 128k and 64k. No interactivity.

BLANKTEST:
- Dumps information about the NMI and VDP status register. Press fire button on controller 1 to toggle between NMI counting and VDP status register polling.

DUMPRAM:
- Displays the contents of startup RAM on the screen, and can be scrolled up and down. Meant to view power up RAM patterns.

MEGATEST:
- emits a byte from 0xC003 of every megacart page, for testing Phoenix Megacart simulation. Builds to differently sized ROMs.

MEMTEST:
- tests the RAM mapping registers for Phoenix SGM simulation. This RAM test is a little more comprehensive.

PORTTEST:
- reads the state of all ports and emits to the screen. Some intelligence is added for the VDP ports.

ROLLERTEST:
- tests the joysticks and roller controller. Holding fire for 3 seconds toggles between joystick and keypad mode. You can also press up or down for some audio tests.

BUILDING:

If you want to rebuild it:

- Install SDCC 3.5.0 (not the latest) 64-bit for Windows from https://sourceforge.net/projects/sdcc/files/sdcc-win64/3.5.0/

- In C:\program files\sdcc\bin, replace sdldz80.exe with my version (provided in tursi_tools)

- Use gnuwin32 or cygwin for Make. I have included the gnuwin32 version but not sure if it works standalone. 

- You might need to change paths in the Makefile. I tried to get them all relative except SDCC.

That should be all you need to make changes. Just type make.

Since it's not a bank-switched cartridge, you can probably build without my hacked linker or "makemegacart.exe", but you'll have to change the makefile's link step.

- crt0.s has the startup code, the vectors, and the raw interrupt handlers. There's some complexity in the spinner handler around my nmi masking code for libti99coleco - mostly wanted to see if it worked. It shouldn't interfere with your testing since the spinner gets priority.

- look at libti99coleco vdp.h if you need different VDP access functions

- if you need to look at the library source code (which I mostly use here only for the VDP access), it's at https://github.com/tursilion/libti99coleco
