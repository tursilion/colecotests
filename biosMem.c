// To build: get the tools and follow the instructions in the Makefile, on Windows,
// then type "make".
//
// Simple two-part test:
// - Verify that 24k ROM space is working
// - Verify that 8k RAM space is working
//

#include "vdp.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

static volatile unsigned char *pRom = (volatile unsigned char *)0;	// access to memory
extern const unsigned char FATFONT[];
volatile __sfr __at 0x54 phRAMBankSelect;       // 32k RAM bank select. 0 and 1 are both 1.
volatile __sfr __at 0x55 phBankingScheme;       // cartridge emulation control (also read it to disable loader)
volatile __sfr __at 0x59 phMegacartMask;        // (NOT AGREED) register to write a page index mask for Megacarts to

// main loop
int main() {
    unsigned char idx,val,ok;
    unsigned int bigidx;
    volatile unsigned char *p;

	// set_text leaves the screen disabled so the user can load character sets,
	// and returns the value to set in VDPR1 to enable it
	unsigned char x = set_text();
	unsigned char col = (COLOR_GRAY<<4)|COLOR_DKBLUE;
	
	// can't load the character set if you're a BIOS program cause there's not BIOS!
    // so we do it ourselves.
    vdpmemcpy(gPattern, FATFONT, 256*8);
	
	// don't need to delay between register writes...
	VDP_SET_REGISTER(VDP_REG_COL, col);
	VDP_SET_REGISTER(VDP_REG_MODE1, x);

    // first let's attempt to determine if there's ROM at 3 x 8k banks
    // and that it's ours, not Coleco's
    // We'll just hexdump from each, I can externally compare
    for (idx=0; idx<20; ++idx) vdpprintf("%X", pRom[idx]);
    for (idx=0; idx<20; ++idx) vdpprintf("%X", pRom[idx+8192]);
    for (idx=0; idx<20; ++idx) vdpprintf("%X", pRom[idx+16384]);
    vdpprintf("\n");

    // now we do a quick RAM test. this program uses 0x6000-0x60FF, and
    // we know there's at least that much. So we'll do a 16-bit test of 0x6100-0x7FFF
    vdpprintf("8k RAM Test...w");
    for (bigidx=0x6100; bigidx<0x7fff; bigidx+=2) {
        *((unsigned int*)&pRom[bigidx]) = bigidx;
    }
    vdpprintf("r");
    for (bigidx=0x6100; bigidx<0x7fff; bigidx+=2) {
        if (*((unsigned int*)&pRom[bigidx]) != bigidx) {
            vdpprintf("\nFail at %X%X (got %X%X)\n", (bigidx>>8), bigidx&0xff, pRom[bigidx+1], pRom[bigidx]);
            break;
        }
    }

    vdpprintf("\n512k RAM test...w");
    // check 256 bytes on each of the 15 available pages, every 16k (for Megacart test to follow)
    // we'll store page number and offset
    phBankingScheme = 0x01;     // upper intram not banked, megacart banking (only to turn off physical cart)
    for (bigidx=0x100; bigidx<0xfff; bigidx+=2) {
        phRAMBankSelect = bigidx>>8;
        *((unsigned int*)&pRom[(bigidx&0xff)+0x8000]) = bigidx;         // main test
        *((unsigned int*)&pRom[(bigidx&0xff)+0xC000]) = bigidx+0x1000;  // second test for later Megacart test
    }
    vdpprintf("r");
    for (bigidx=0x100; bigidx<0xfff; bigidx+=2) {
        phRAMBankSelect = bigidx>>8;
        if (*((unsigned int*)&pRom[(bigidx&0xff)+0x8000]) != bigidx) {
            vdpprintf("\nFail at %X%X (got %X%X)\n", (bigidx>>8), bigidx&0xff, pRom[bigidx+0x8001], pRom[bigidx+0x8000]);
            break;
        }
    }

    vdpprintf("\nMegacart test...r\n");
    phBankingScheme = 0x11;     // rom emulation with banking, megacart banking scheme
    phMegacartMask = 0x1f;      // max 512k space
    // check the fixed page first
    // we only care about the page byte, so we don't need all the int stuff...
    if (pRom[0x8001] != 0x1f) {    // last page (0x0f) plus the 0x1000 offset
        vdpprintf("Fixed page fail... expected 0x1f got %X\n", pRom[0x8001]);
    }
    // megacart should have 30 pages bankable into 0xC000 by reading 0xFFC0 and up (really 0xFFE2 for us)
    val = 0x1F;
    ok=1;
    for (p=(unsigned char*)0xffff; p>(unsigned char*)0xffe1; --p) {
        // we only care about the page byte, so we don't need all the int stuff...
        idx = *p;   // do the bank
        if (pRom[0xC001] != val) {
            //         -------------------- 
            vdpprintf("%X%X (%X != %X)     ", (((int)p)>>8), ((int)p)&0xff, pRom[0xC001], val);
            ok=0;
        }
        val^=0x10;  // toggle the first bit
        if (val & 0x10) --val;  // decrement every other toggle
    }
    if (ok) {
        // go ahead and also try the mask - we skip this if the above failed
        // here we just test that specific banks are mirrored as we expect
        vdpprintf("Test 256k..");
        phMegacartMask = 0x0f;  // max 256k
        idx = *(volatile unsigned char*)0xffff;  // bank 0
        val = pRom[0xc001];

        p = (unsigned char *)0xffef;    // bank 16
        idx = *p;
        if (pRom[0xc001] != val) {
            vdpprintf("%X%X (%X != %X)", (((int)p)>>8), ((int)p)&0xff, pRom[0xC001], val);
        }
        vdpprintf("\n");

        vdpprintf("Test 128k...");
        phMegacartMask = 0x07;  // max 128k
        p = (unsigned char *)0xffef;    // bank 16
        idx = *p;
        if (pRom[0xc001] != val) {
            vdpprintf("%X%X (%X != %X) ", (((int)p)>>8), ((int)p)&0xff, pRom[0xC001], val);
        }
        p = (unsigned char *)0xfff7;    // bank 8
        idx = *p;
        if (pRom[0xc001] != val) {
            vdpprintf("%X%X (%X != %X)", (((int)p)>>8), ((int)p)&0xff, pRom[0xC001], val);
        }
        vdpprintf("\n");

        vdpprintf("Test 64k...");
        phMegacartMask = 0x03;  // max 64k
        p = (unsigned char *)0xffef;    // bank 16
        idx = *p;
        if (pRom[0xc001] != val) {
            vdpprintf("%X%X (%X != %X) ", (((int)p)>>8), ((int)p)&0xff, pRom[0xC001], val);
        }
        p = (unsigned char *)0xfff7;    // bank 8
        idx = *p;
        if (pRom[0xc001] != val) {
            vdpprintf("%X%X (%X != %X) ", (((int)p)>>8), ((int)p)&0xff, pRom[0xC001], val);
        }
        p = (unsigned char *)0xfffb;    // bank 4
        idx = *p;
        if (pRom[0xc001] != val) {
            vdpprintf("%X%X (%X != %X)", (((int)p)>>8), ((int)p)&0xff, pRom[0xC001], val);
        }
        vdpprintf("\n");
    }

	// nothing more to do
	vdpprintf("\n\n** DONE **");
	
	// spin forever
	for (;;) { }

	return 0;
}
