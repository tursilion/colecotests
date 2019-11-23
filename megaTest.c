// To build: get the tools and follow the instructions in the Makefile, on Windows,
// then type "make".
//
// Simple Megacart Test - an external tool builds all the variations
// 

#include "vdp.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

// hardware control - pRom[] is terribly inefficient in SDCC but easy to write ;)
static volatile unsigned char *const pRom = (volatile unsigned char * const)0;	// access to memory
// paging control
static volatile unsigned char *const pPage = (volatile unsigned char * const)0xff00;	// access to memory
// what's the size of the cart, hacked into the header by the build tool
unsigned char lastPage, page, outPage, line;
int size;

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

    // how big are we?
    lastPage = pRom[0x8002];
    
    // calculate a size for printing
    size=16;    // one page is 16k
    page = 0xff;
    while (page != lastPage) {
        size+=16;
        --page;
    }

    // print the welcome banner
	vdpprintf("Megacart test for %c%c%ck carts\n\n",
              // gonna print in decimal!
              (size<100)?' ':(size/100)+'0',
              ((size%100)/10)+'0',          // we know at least 2 digits
              ((size%100)%10)+'0'
              );

    // start testing... these start the same but may diverge after the lastPage
    page = 0xff;
    outPage = 0xff;
    // 0xFFC0 is the last banking page, but the last two pages (of 512k) do not work on Phoenix
    // Since this is a 1MB banking range, even 512k mirrors once.
    // there are up to 32 pages, so we'll just print two columns
    while (outPage > 0xbf) {
        x = pPage[outPage];             // bank it
        x = pRom[0xC003];               // get the byte
        vdpprintf("FF%X=%X", outPage, x);  // print result
        if ((page == 0xe0) || (page == 0xe1)) {
            // these pages are not available on Phoenix
            vdpprintf("*  ");
        } else {
            if (x != page) {
                vdpprintf("!! ");
            } else {
                vdpprintf("   ");
            }
        }
        --outPage;
        --page;
        if (page < lastPage) page = 0xff;   // reset on wrap
    }

	// nothing more to do
    vdpprintf("\n* = not verified, unavailable on Phoenix\n");
	vdpprintf("\n** DONE **");
	
	// spin forever
	for (;;) { }

	return 0;
}
