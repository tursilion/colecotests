// To build: get the tools and follow the instructions in the Makefile, on Windows,
// then type "make".
//
// Simple RAM dump to screen - joy up/down to scroll
// NOTE: /uses/ RAM, so be aware of that.
// 

#include "vdp.h"
#include "kscan.h"

// main loop
int main() {
	// set_text leaves the screen disabled so the user can load character sets,
	// and returns the value to set in VDPR1 to enable it
	unsigned char x = set_text();
	unsigned char col = (COLOR_GRAY<<4)|COLOR_DKBLUE;
	unsigned char *adr = (unsigned char*)0x7000;
	int i;
	
	// load the lowercase character set because I like it...
	charsetlc();
	
	// don't need to delay between register writes...
	VDP_SET_REGISTER(VDP_REG_COL, col);
	VDP_SET_REGISTER(VDP_REG_MODE1, x);
	
	// really dumb, really hacky, but functional
	for (;;) {
		VDP_SET_ADDRESS_WRITE(0);
		// now, we can write 960/2 bytes
		for (i=0; i<960/2; ++i) {
			// should be enough delay in the math
			faster_hexprint(*(adr+i));
		}
		
		// now check the joystick
		joystfast(1);
		if (KSCAN_JOYY & 0x80) {
			if (adr > (unsigned char*)0x7000) adr-=20;
		} else if (KSCAN_JOYY > 0) {
			if (adr < (unsigned char*)(0x7400-19)) adr+=20;
		}
	}

	return 0;
}
