# install SDCC 3.5.0 64-bit for Windows, and then in C:\program files\sdcc\bin,
# replace sdldz80.exe with Tursi's version. use gnuwin32 or cygwin for Make, and
# provide Tursi's "makeMegaCart.exe" for the linking.

CC = "c:/program files/sdcc/bin/sdcc"
CFLAGS = -mz80 -c "-I./libti99coleco" --std-sdcc99 --vc -DENABLEFX --opt-code-speed
AS = "c:/program files/sdcc/bin/sdasz80"
AR = "c:/program files/sdcc/bin/sdcclib"
AFLAGS = -plosgff
RM = del /F 
# might need to use o for older SDCC, rel for newer
EXT=rel

# Recipe to compile the app (pad biosmem to be >16k)
all: rollerTest memTest biosMem megaTest portTest
	"tursi_tools/makemegacart.exe" rollerTest.ihx rollerTest.rom
	"tursi_tools/makemegacart.exe" portTest.ihx portTest.rom
	"tursi_tools/makemegacart.exe" memTest.ihx memTest.rom
	"tursi_tools/makemegacart.exe" megaTest.ihx megaTest.rom
	"tursi_tools/makemegacart.exe" -bios biosMem.ihx biosMem.rom

rollerTest: rollerTest.$(EXT) crt0_roller.$(EXT)
	$(CC) -mz80 --no-std-crt0 --code-loc 0x8100 --data-loc 0x7000 -l./libti99coleco/libti99.a "./crt0_roller.$(EXT)" rollerTest.$(EXT) -o rollerTest.ihx

portTest: portTest.$(EXT) crt0_roller.$(EXT)
	$(CC) -mz80 --no-std-crt0 --code-loc 0x8100 --data-loc 0x7000 -l./libti99coleco/libti99.a "./crt0_roller.$(EXT)" portTest.$(EXT) -o portTest.ihx

memTest: memTest.$(EXT) crt0_memtest.$(EXT)
	$(CC) -mz80 --no-std-crt0 --code-loc 0x8100 --data-loc 0x7000 -l./libti99coleco/libti99.a "./crt0_memtest.$(EXT)" memTest.$(EXT) -o memTest.ihx

megaTest: megaTest.$(EXT) crt0_memtest.$(EXT)
	$(CC) -mz80 --no-std-crt0 --code-loc 0x8100 --data-loc 0x7000 -l./libti99coleco/libti99.a "./crt0_memtest.$(EXT)" megaTest.$(EXT) -o megaTest.ihx

biosMem: biosMem.$(EXT) dummy.$(EXT) mattfont.$(EXT) crt0_bios.$(EXT)
	$(CC) -mz80 --no-std-crt0 --code-loc 0x0090 --data-loc 0x6000 -l./libti99coleco/libti99.a "./crt0_bios.$(EXT)" biosMem.$(EXT) mattfont.$(EXT) dummy.$(EXT) -o biosMem.ihx

# special to put dummy in its own segment
dummy.rel: dummy.c
	$(CC) $(CFLAGS) -o dummy.rel dummy.c --codeseg PADDING --constseg PADDING

# Recipe to clean all compiled objects
.phony clean:
	$(RM) *.rel *.map *.lst *.lnk *.sym *.asm *~ *.o *.obj *.ihx *.sprite.* *.rom *.rel *.a *.lib

# Recipe to compile all C files
%.rel: %.c
	$(CC) -c $< $(CFLAGS) -o $@

# Recipe to compile all assembly files
%.rel: %.s
	$(AS) -o $@ $<
