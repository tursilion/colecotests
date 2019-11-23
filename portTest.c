// To build: get the tools and follow the instructions in the Makefile, on Windows,
// then type "make".
// just reads the ports, ignores modes or side effects

#include "vdp.h"

static volatile __sfr __at 0xfc joy1Read;		// read here to read joystick 1

unsigned char getPort(unsigned char port) {
	__asm
	push bc		; save c reg
	ld hl,#4	; going to get pointer to stack
	add hl,sp	; make stack pointer in hl
	ld c,(hl)	; read port into c
	in a,(c)	; read the hardware port c into a
	ld l,a		; move a to the return register l
	pop bc		; get original c back
	ret			; real return
	__endasm;
	
	return port;	// make sdcc happy
}

// dummy function to satisfy the crt0
void spinnerInt() {
	__asm
	nop
	__endasm;
}
	
// main loop
int main() {
	// set_graphics leaves the screen disabled so the user can load character sets,
	// and returns the value to set in VDPR1 to enable it
	unsigned char x = set_graphics(VDP_SPR_8x8);
	unsigned char col = (COLOR_GRAY<<4)|COLOR_DKBLUE;
	unsigned char port = 0;
	unsigned char cnt = 0;
	unsigned int nextscrn = 0;
	
	// set up the color tables
	vdpmemset(gColor, 0xe0, 32);
	
	// load a character set
	charset();
	
	// don't need to delay between register writes...
	VDP_SET_REGISTER(VDP_REG_COL, col);
	VDP_SET_REGISTER(VDP_REG_MODE1, x);

	// update the ports
	while (1) {
		unsigned char x = getPort(port);
		
		if (cnt == 0) {
			// need this because there's one wrap case where just
			// skipping the space won't work, cause it's at the end of the line
			VDP_SET_ADDRESS_WRITE(nextscrn);
			nextscrn += 32;
		}
		
		faster_hexprint(x);
		
		// as far as alignment goes, we generate 10 columns, and spaces after the
		// first nine. That works great until we read from the VDP data ports - that
		// causes an addition increment. We need to take those addresses into account.
		// The basic address is 0xBE, so the range is 0xA0-0xBF, and it's even addresses
		// only (odd addresses read status). The port increments first because of where
		// the space lands
		++port;
		if (cnt < 10) {
			// to align nicely, only 10 columns
			// This should skip the extra write for VDPRD
			if ((port & 0xE1) != 0xA0) {
				VDPWD = ' ';
			}
			++cnt;
		} else {
			cnt = 0;
		}
		
		if (port == 0) {
			// they don't wrap together
			cnt = 0;
			nextscrn = 0;
		}
		
		// and repeat forever!
	}

//	return 0;
}
