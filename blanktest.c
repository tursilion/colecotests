// To build: get the tools and follow the instructions in the Makefile, on Windows,
// then type "make".
//
// Displays the status of the NMI from the VDP as well as the status register itself.
// We count ticks and increment once per second.
// 
// Warning that this uses the roller CRT0.

#include "vdp.h"

static volatile __sfr __at 0xfc joy1Read;		// read here to read joystick 1
//static volatile __sfr __at 0xff joy2Read;		// read here to read joystick 2
static volatile __sfr __at 0x80 keySelect;		// write here to select keypad
static volatile __sfr __at 0xc0 joySelect;		// write here to select joystick
static volatile __sfr __at 0xff SOUND;			// audio out port

// variables...
volatile unsigned char mode = 0;	 // hold fire for a few seconds to toggle mode (0=NMI, 1=polling)
volatile unsigned int intCnt = 0;	 // interrupt counter
volatile unsigned int pollCnt = 0;  // poll counter
unsigned char modeCnt = 0;			 // counter for holding fire (not used by interrupt)
volatile unsigned char changeMode = 0;		 // forces mode change on int

// This code does the joystick reading inline, not via the lib
// it only concerns itself with the first joystick, and it does
// care about spinner interrupts (see crt0.s)
void setJoy() {
	joySelect = 0;		// select joystick mode
	__asm				// online docs recommend 3 NOPs after a select for Adam compatibility
	nop
	nop
	nop
	__endasm;
}
void setKey() {
	keySelect = 0;		// select keypad mode
	__asm				// online docs recommend 3 NOPs after a select for Adam compatibility
	nop
	nop
	nop
	__endasm;
}

// spinner interrupt
// WARNING: this can be interrupted by the NMI, which is probably why it's so
// hard to get it right...
void spinnerInt() {
}

// poll function
void pollHook() {
	++pollCnt;
	
	vdpmemcpy(gImage + 96 + 5, "Poll: ", 6);
	faster_hexprint(pollCnt>>8);
	faster_hexprint(pollCnt&0xff);
}
	
// NMI user funct
void nmiHook() {
	++intCnt;
	
	vdpmemcpy(gImage + 64 + 5, "NMIs: ", 6);
	faster_hexprint(intCnt>>8);
	faster_hexprint(intCnt&0xff);

	if (VDP_STATUS_MIRROR & VDP_ST_INT) {
		// this SHOULD happen every time!
		pollHook();
	}
	
	if (changeMode) {
		mode = 1;
		VDP_SET_REGISTER(VDP_REG_MODE1, 0xc0);	// no interrupt
		changeMode = 0;
	}
}

// main loop
int main() {
	// set_graphics leaves the screen disabled so the user can load character sets,
	// and returns the value to set in VDPR1 to enable it
	unsigned char x = set_graphics(VDP_SPR_8x8);
	unsigned char col = 0xe4;
	
	// load the lowercase character set because I like it...
	charsetlc();
	
	// set up the color tables
	vdpmemset(gColor, 0xe0, 32);
	
	// don't need to delay between register writes...
	VDP_SET_REGISTER(VDP_REG_COL, col);
	VDP_SET_REGISTER(VDP_REG_MODE1, x);
	
	// not racey as long as we disable our flag
	VDP_INT_DISABLE;
	changeMode = 0;
	setUserIntHook(nmiHook);
	
	// make sure we start in NMI mode
	mode = 0;
	setJoy();
	x = VDPST;			// clear any old pending interrupt
	
	writestring(5, 0, "In NMI mode, both counters      should count");
	writestring(8, 0, "In Poll mode, only poll should  count");
	writestring(11, 0, "Press FIRE to change mode.");

	while (1) {
		unsigned char x;
		
		// check interrupts
		VDP_INT_POLL;

		if (mode != 0) {
			// polling the status register, else the NMI does all the work
			x = VDPST;
			if (x & VDP_ST_INT) {
				pollHook();
			}
		}
		
		// display the mode we're in (faster to write this way than use string functions)
		if (mode==0) {
			vdpmemcpy(gImage + 32 + 5, "NMI Mode ", 9);
		} else {
			vdpmemcpy(gImage + 32 + 5, "Poll Mode", 9);
		}
		
		// Check for mode change
		x = joy1Read;
		
		// Now check if we should change modes for next frame
		// bit 0x40 is always one fire button or the other...
		if (!(x & 0x40)) {
			++modeCnt;
			if (modeCnt >= 180) {
				// timeless!
				modeCnt = 0;
				if (mode == 0) {
					// do it on the NMI for safety
					changeMode = 1;
				} else {
					// can't rely on pollHook being called, that's what we are testing
					mode = 0;
					VDP_SET_REGISTER(VDP_REG_MODE1, 0xe0);	// interrupt back on
				}
			}
		} else {
			// reset the counter if no fire pressed
			modeCnt = 0;
		}
		
		// and repeat forever!
	}

//	return 0;
}
