// To build: get the tools and follow the instructions in the Makefile, on Windows,
// then type "make".
//
// Simple RAM test for the SGM mapping bits
// 
// Assumptions:
//
// Port 0x60, value >02 (0x60 through 0x7f should all the same)
// >0000 to >1FFF -- 0=RAM (0x0D), 2=BIOS (0x0F) (but ONLY if port 53 bit >01 is ON)
//
// Port 0x53, value >01
// >2000 to >5FFF -- 0=unused, 1=16k RAM
// >6000 to >7FFF -- 0=1k rpt, 1=8k RAM (distinct 8k from the 1k block)
//
// There are two large issues with this test.
// 1) C runtime requires RAM for stack and variables - the bank switch affects this
//    - to counter this, we reserve 256 bytes for C and exclude those bytes in every page
// 2) All interrupts run through the BIOS, which will cause crashes when we switch
//    - to counter this, our CRT0 ignores both spinner and nmi interrupts.
//

#include "vdp.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

// macros to backup and restore RAM to VDP
// REMEMBER that this ALSO swaps ALL local variables AND THE STACK
// use them intelligently or pay the price. ;)
#define RESTORERAM							\
	{										\
		int cnt;							\
		unsigned char *pDest;				\
		cnt = 256;							\
		pDest = (unsigned char*)0x6000;	\
		VDP_SET_ADDRESS(0x1800);			\
		while (cnt--) {						\
			*(pDest++)=VDPRD;				\
		}									\
	}
	
#define BACKUPRAM							\
	{										\
		int cnt;							\
		unsigned char *pDest;				\
		cnt = 256;							\
		pDest = (unsigned char*)0x6000;	\
		VDP_SET_ADDRESS_WRITE(0x1800);		\
		while (cnt--) {						\
			VDPWD=*(pDest++);				\
		}									\
	}


// hardware control
static volatile __sfr __at 0x7f biosCtrl;		// port that controls the BIOS mapping (0x60-0x7f)
static volatile __sfr __at 0x55 ldrCtrl;        // port that controls the loader/banking (Phoenix specific)
static volatile __sfr __at 0x53 ramCtrl;		// port that controls the 24k RAM mapping (SGM specific)
static volatile unsigned char *pRom = (volatile unsigned char *)0;	// access to memory

// There are important bits other than these, at least for the Adam, so
// these values have importance.
// The least significant 4 bits control the mapping, 2 bits for lower 32k,
// 2 bits for upper 32k
// ..00 = lower BIOS ROM
// ..01 = 32k internal RAM
// ..10 = 32k expansion RAM
// ..11 = OS7 (ColecoVision) ROM plus 24k internal RAM
// 00.. = upper 32k internal RAM
// 01.. = expansion ROM
// 10.. = 32k expansion RAM
// 11.. = cartridge ROM
//
// BIOS_ON then defines upper 32k cart space, lower 32k Coleco BIOS plus RAM.
// However, on the SGM only bit 0x02 is meaningful, where 0x02 means OS7 ROM,
// and 0x00 means 8k RAM. This means the values are only trivially compatible
// with the Adam.
// It looks like ColEm honors the upper 2 bits for banking, I doubt that is correct
// for the SGM, but as long as we use these "valid" values, it should be fine.
#define BIOS_ON 0x0F
// BIOS_OFF depends upper 32k cart space, lower 32k all RAM.
// But on SGM it's only 8k RAM in the BIOS space
#define BIOS_OFF 0x0D

// some variables - keep it minimal so we can test most of the RAM at 0x6000
unsigned char oldbyte;
#define firstVal 0x12
#define nextVal 0x34

// fill a block of memory with a starting value
// note that if BIOS banking exists, this would switch it
void fillRam(unsigned int start, unsigned int count, unsigned char val) {
	// we skip over the first 256 bytes of each 1k
	// to preserve the RAM that C itself uses
	while (count-- > 0) {
		if ((start&0x3ff) >= 0x100) {
			pRom[start++]=val++;
		} else {
			++start;
			++val;
		}
	}
}

// test a block of RAM - return true if it passes, false if it doesn't
unsigned char testRam(unsigned int start, unsigned int count, unsigned char val) {
	// we skip over the first 256 bytes of each 1k
	// to preserve the RAM that C itself uses
	while (count-- > 0) {
		if ((start&0x3ff) >= 0x100) {
			if (pRom[start++] != val++) {
				return false;
			}
		} else {
			++start;
			++val;
		}
	}
	return true;
}

// test the ColecoVision - if testFill is false, skip the fill step
void testBase(unsigned char testFill) {
	if (testFill) {
		vdpprintf("Setting machine to default ColecoVision\n");
	}
	ramCtrl = 0x00;			// 1k RAM
	biosCtrl = BIOS_ON;		// BIOS enabled
	
	// check if there's a BIOS at >0000 (we just check a few bytes)
	// The original BIOS had a copyright string at >14B6, seems like
	// a good place to check.
	oldbyte = pRom[0x14b6];
	if (pRom[0x14b6] != pRom[0x14b7]) {
		vdpprintf("+ Looks like there is a ROM\n");
	} else {
		vdpprintf("x Is BIOS missing? (inconclusive)\n");
	}
	
	if (testFill) {
		// see if it banks (Matt hasn't built this yet)
		// this can only work once without a reset, so inside the testFill block

        // turn off the Phoenix boot loader
        int x = ldrCtrl;    // reading from port 0x55 should cause a switch

		if (oldbyte != pRom[0x14b6]) {
			vdpprintf("+ ROM bank switch - Coleco %c%c%c%c\n", pRom[0x14b6], pRom[0x14b7], pRom[0x14b8], pRom[0x14b9]);
	        // update oldbyte for the later RAM switch test
            oldbyte = pRom[0x14b6];
		} else {
			vdpprintf("x ROM did not bank- Coleco %c%c%c%c\n", pRom[0x14b6], pRom[0x14b7], pRom[0x14b8], pRom[0x14b9]);
		}
		
		// fill RAM -- which should of course change nothing except in the 1k block
		vdpprintf("Filling RAM...");
		fillRam(0x0000, 0x2000, firstVal);
		vdpprintf("1.");
		fillRam(0x2000, 0x2000, firstVal+1);
		vdpprintf("2.");
		fillRam(0x4000, 0x2000, firstVal+2);
		vdpprintf("3.");
		fillRam(0x6000, 0x2000, firstVal+3);
		vdpprintf("4\n");
	}
	
	// test RAM in each block, except the 1k RAM, which we'll treat special
	// firstVal loops every 256 bytes, so we can repeat it
	vdpprintf("Testing memory layout...\n");
	if (testRam(0x0000, 0x2000, firstVal)) {
		vdpprintf("x Unexpected RAM at 0x0000\n");
	}
	if (testRam(0x2000, 0x2000, firstVal+1)) {
		vdpprintf("x Unexpected RAM at 0x2000\n");
	}
	if (testRam(0x4000, 0x2000, firstVal+2)) {
		vdpprintf("x Unexpected RAM at 0x4000\n");
	}
	// special case for the 1k RAM, it should repeat over this range
	// we'll do the dumb test first, cause it's easy
	if (!testRam(0x6000, 0x2000, firstVal+3)) {
		vdpprintf("x RAM test at 0x6000 failed\n");
	}
	// now make sure it repeats. To do that, we have to break the pattern
	pRom[0x6100]=0;		// firstVal is not 0
	// make sure it's echoed at each point
	if ((pRom[0x6100] != 0) ||
		(pRom[0x6500] != 0) ||
		(pRom[0x6900] != 0) ||
		(pRom[0x6D00] != 0) ||
		(pRom[0x7100] != 0) ||
		(pRom[0x7500] != 0) ||
		(pRom[0x7900] != 0) ||
		(pRom[0x7D00] != 0)) {
		vdpprintf("x 1k RAM mirror at 0x6000 incorrect.\n");
	}
	// change it back just to be consistent later
	pRom[0x6100]=firstVal+3;
	vdpprintf("Base test complete.\n\n");
}

void testSgm(unsigned char testFill) {
	// the current theory is that the ramCtrl is actually a global
	// enable bit for the SGM, and BIOS can't be switched out before that.
	// The ColecoVision schematic shows that memory disable is all or nothing
	
	// to do this switch, we need to copy the RAM that C uses before C needs any of it
	// since we can't get any other RAM before we lose our 1k,
	// we'll use VDP RAM. I think we can get away with being really
	// sneaky and using the lib, even though the stack will look funny
	// we want to test the BIOS separately
	vdpprintf("Setting SGM RAM...");
	BACKUPRAM;
	ramCtrl = 0x01;							// 24k additional RAM
	RESTORERAM;
	vdpprintf("ok\n");
	
	// make sure the BIOS is still here
	if (oldbyte != pRom[0x14b6]) {
		// this isn't perfect, of course, but it's probably good enough
		vdpprintf("x BIOS switched out before port 0x7f.\n");
	}
	
	// now switch the BIOS
	vdpprintf("Setting BIOS to RAM...\n");
	biosCtrl = BIOS_OFF;					// BIOS disabled, 8k RAM in place

	// check if BIOS became RAM
	if (testFill) {
		vdpprintf("Filling RAM...");
		fillRam(0x0000, 0x2000, nextVal);
		vdpprintf("1.");
		fillRam(0x2000, 0x2000, nextVal+1);
		vdpprintf("2.");
		fillRam(0x4000, 0x2000, nextVal+2);
		vdpprintf("3.");
		fillRam(0x6000, 0x2000, nextVal+3);
		vdpprintf("4\n");
	}
	// now check each block
	if (!testRam(0x0000, 0x2000, nextVal)) {
		vdpprintf("x RAM test at 0x0000 failed.\n");
	}
	if (!testRam(0x2000, 0x2000, nextVal+1)) {
		vdpprintf("x RAM test at 0x2000 failed.\n");
	}
	if (!testRam(0x4000, 0x2000, nextVal+2)) {
		vdpprintf("x RAM test at 0x4000 failed.\n");
	}
	if (!testRam(0x6000, 0x2000, nextVal+3)) {
		vdpprintf("x RAM test at 0x6000 failed.\n");
	}
	// make sure that the RAM at 0x6000 is /not/ mirrored
	pRom[0x6100]=0;		// nextVal is not 0-3
	// make sure it's NOT echoed at each point
	if ((pRom[0x6100] != 0) ||
		(pRom[0x6500] == 0) ||
		(pRom[0x6900] == 0) ||
		(pRom[0x6D00] == 0) ||
		(pRom[0x7100] == 0) ||
		(pRom[0x7500] == 0) ||
		(pRom[0x7900] == 0) ||
		(pRom[0x7D00] == 0)) {
		// note this might only be a partial mirror - can dig deeper if you see it
		vdpprintf("x 8k RAM at 0x6000 seems to be mirrored.\n");
	}
	// change it back just to be consistent later
	pRom[0x6100]=nextVal+3;
	vdpprintf("SGM test complete.\n\n");
}
	
// main loop
int main() {
	// set_text leaves the screen disabled so the user can load character sets,
	// and returns the value to set in VDPR1 to enable it
	unsigned char x = set_text();
	unsigned char col = (COLOR_GRAY<<4)|COLOR_DKBLUE;
	
	// load the lowercase character set because I like it...
	charsetlc();
	
	// don't need to delay between register writes...
	VDP_SET_REGISTER(VDP_REG_COL, col);
	VDP_SET_REGISTER(VDP_REG_MODE1, x);

	// All right, start by forcing the machine to the default state 
	// (if there's no SGM this will work anyway)
	// We assume that we are already in this state - if we are not,
	// we will crash as we lose our stack
	testBase(true);
	
	// now try the SGM mode - this will copy the stack frame to survive
	testSgm(true);
	
	// switch back to the default mapping and do a quick test it's NOT RAM
	// This is an inverted test compared to the function
	vdpprintf("Switching back to base ColecoVision...");
	BACKUPRAM;
	biosCtrl = BIOS_ON;		// BIOS enabled
	ramCtrl = 0x00;			// 1k RAM
	RESTORERAM;
	
	vdpprintf("\nTesting memory retention...\n");
	if (testRam(0x0000, 0x2000, nextVal)) {
		vdpprintf("x Memory at 0x0000 is still RAM.\n");
	}
	if (testRam(0x2000, 0x2000, nextVal+1)) {
		vdpprintf("x Memory at 0x2000 is still RAM.\n");
	}
	if (testRam(0x4000, 0x2000, nextVal+2)) {
		vdpprintf("x Memory at 0x4000 is still RAM.\n");
	}
	if (testRam(0x6000, 0x2000, nextVal+3)) {
		vdpprintf("x Memory at 0x6000 contains SGM RAM.\n");
	}
	
	// repeat the Coleco tests - this reverts to the old stack frame
	// Nope that opcode recommends you do NOT revert back to the
	// standard layout on port 0x53, but it's not clear if there
	// is a technical reason for this.
	testBase(false);
	
	// Finally change back to the SGM again and do one last test 
	// that data was retained
	testSgm(false);
	
	// nothing more to do
	vdpprintf("** DONE **");
	
	// spin forever
	for (;;) { }

	return 0;
}
