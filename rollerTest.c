// To build: get the tools and follow the instructions in the Makefile, on Windows,
// then type "make".
//
// 1. Constant keypad or joystick selection.  
// Selecting keypad vs stick should only occur from a specific input, 
// like holding either fire button for 3 seconds or so.
// 
// 2. No interpretation of the data.  Just display each bit from each 
// byte as it is read from the controller port.
//
// 3. Indicate when the controller interrupt, i.e. the maskable interrupt, 
// has triggered.  My thought was to just have a counter that is incremented 
// in the ISR and displayed in the main loop.
//
// The interrupt "type" you want for the maskable interrupt is "1" 
// (there are 3 "types" for the maskable interrupt in the Z80, and the type 
// is set as part of the assembly instruction that enables the interrupt).  
// The CPU vector is >0038 I believe, which certainly jumps to cartridge space 
// (I don't know the address).
//
// The actual switches-to-bits in the bytes received are:
//
// A,B = quadrature A and B
// F = Fire
// X = not defined, this is the top of a timing capacitor and will not be useful
//
// 7 6 5 4 3 2 1 0
// A F B X L D R U
//
// >01 - stick up / keypad bit (from pin 1)
// >02 - stick right / keypad bit (from pin 4)
// >04 - stick down / keypad bit (from pin 2)
// >08 - stick left / keypad bit (from pin 3)
// >10 - unused, analog capacitor data
// >20 - Quadrature B input (from pin 7)
// >40 - stick left fire / keypad right fire/arm (from pin 6)
// >80 - Quadrature A input (from NAND output, triggered by pin 9, causes interrupt on falling edge)
//
// In keypad mode, combinations of bits are returned

#include "vdp.h"

static volatile __sfr __at 0xfc joy1Read;		// read here to read joystick 1
//static volatile __sfr __at 0xff joy2Read;		// read here to read joystick 2
static volatile __sfr __at 0x80 keySelect;		// write here to select keypad
static volatile __sfr __at 0xc0 joySelect;		// write here to select joystick
static volatile __sfr __at 0xff SOUND;			// audio out port

// variables...
volatile unsigned char mode = 0;	// hold fire for a few seconds to toggle mode (0=joy,1=key)
volatile unsigned char pos = 128; 	// sprite position (just to see if I can read the position)
volatile unsigned char intCnt = 0;	// interrupt counter
unsigned char modeCnt = 0;			// counter for holding fire (not used by interrupt)

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
	unsigned char x;
	
	// update the counter
	++intCnt;
	
	// so what do you do in keyboard mode? as a test I'll try switching back to read direction
	// anyway. Since we haven't cleared the interrupt yet, it shouldn't cause another pulse...?
	if (mode) {
		// I want the NOPs in this case
		setJoy();
	}
	
	x = joy1Read;
	// we know quad A is falling, so we check quad B to get direction
	if (x&0x20) {
		// clockwise?
		++pos;
	} else {
		// counter-clockwise?
		--pos;
	}
	
	// if we're in keyboard mode, switch back before we return
	if (mode) {
		keySelect = 0;
		// there will be enough delay before anything else triggers
	}
}
	
// main loop
int main() {
	// set_graphics leaves the screen disabled so the user can load character sets,
	// and returns the value to set in VDPR1 to enable it
	unsigned char x = set_graphics(VDP_SPR_8x8);
	unsigned char col = (COLOR_GRAY<<4)|COLOR_DKBLUE;
	
	// load the lowercase character set because I like it...
	charsetlc();
	
	// set up the color tables
	vdpmemset(gColor, 0xe0, 32);
	
	// and set up a little sprite to roll side to side...
	sprite(0, '*', COLOR_WHITE, 96, pos);
	VDP_SAFE_DELAY();	// need to manually re-terminate the sprite list,
	VDPWD = 0xd0;		// VDP address is still valid from the sprite() call

	// don't need to delay between register writes...
	VDP_SET_REGISTER(VDP_REG_COL, col);
	VDP_SET_REGISTER(VDP_REG_MODE1, x);
	
	// make sure we start in joystick mode
	mode = 0;
	setJoy();

	while (1) {
		unsigned char x, mask, i;
		
		// update screen on vint		
		vdpwaitvint();
		
		// move the sprite (I know it's sprite 0, 'x' is the second byte of the table)
		vdpchar(gSprite+1, pos);
		
		// display the mode we're in (faster to write this way than use string functions)
		if (mode) {
			vdpmemcpy(gImage + 32 + 5, "Key: ", 5);
		} else {
			vdpmemcpy(gImage + 32 + 5, "Joy: ", 5);
		}
		
		// read and display the inputs
		// VDP address is still valid from the vdpmemcpy
		x = joy1Read;
		
		mask = 0x80;
		for (i = 0; i<8; ++i) {
			if (x & mask) {
				VDPWD = '1';
			} else {
				VDPWD = '0';
			}
			VDP_SAFE_DELAY();
			VDPWD = ' ';
			VDP_SAFE_DELAY();
			mask >>= 1;
		}
		
		// display the interrupt count
		vdpmemcpy(gImage + 64 + 5, "Ints: 0x", 8);
		faster_hexprint(intCnt);
		
		// Now check if we should change modes for next frame
		// bit 0x40 is always one fire button or the other...
		if (!(x & 0x40)) {
			++modeCnt;
			if (modeCnt >= 180) {
				// roughly 3 seconds
				mode = !mode;
				modeCnt = 0;
				if (mode) {
					setKey();
				} else {
					setJoy();
				}
			}
		} else {
			// reset the counter if no fire pressed
			modeCnt = 0;
		}
		
		// alternate world hack - check sound. UP for 1, DOWN for 0,
		// and neither for mute
		if ((x&1)==0) {
			// up - count of 1
			SOUND=0x81;
			SOUND=0x00;
			SOUND=0x90;
		} else if ((x&4)==0) {
			// down - count of 0
			SOUND=0x80;
			SOUND=0x00;
			SOUND=0x90;
		} else {
			// neither - mute
			SOUND=0x9F;
		}
		
		// and repeat forever!
	}

	return 0;
}
