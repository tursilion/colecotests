; crt0.s for Phoenix ColecoVision Boot BIOS
; This is the actual BIOS version

	.module crt0
	.globl _main
	.globl _vdpinit
	.globl _my_nmi
	.globl _vdpLimi
    .globl  l__INITIALIZER
    .globl  s__INITIALIZED
    .globl  s__INITIALIZER

	.area _HEADER(ABS)
	.org 0x0000

; Boot starts here
    ld sp,#0x6100           ; set stack (unnecessarily)
    jp start                ; and jump to the startup code
    .db 0xff, 0xff          ; padding

; These are vectors into the cartridge, normally.
; But, we dont actually WANT to run a cartridge, so we
; will just jump to a dummy stub function that does nothing.
; We swap out to the real BIOS before we boot, so that
; wont even matter. Note these are location sensitive,
; so dont change the code without verifying addresses.

RST_8H:
    JP dummyIRQ
    .db 0xff, 0xff, 0xff, 0xff, 0xff

RST_10H:
    JP dummyIRQ
    .db 0xff, 0xff, 0xff, 0xff, 0xff

RST_18H:
    JP dummyIRQ
    .db 0xff, 0xff, 0xff, 0xff, 0xff

RST_20H:
    JP dummyIRQ
    .db 0xff, 0xff, 0xff, 0xff, 0xff

RST_28H:
    JP dummyIRQ
    .db 0xff, 0xff, 0xff, 0xff, 0xff

RST_30H:
    JP dummyIRQ
    .db 0xff, 0xff, 0xff, 0xff, 0xff

dummyIRQ:
    ei
    reti
    .db 0xff, 0xff, 0xff, 0xff, 0xff

; Just some random noise to pad
    .ascii 'Deae Lunae: Benedicite noctes decora..'

nmi:
    jp run_nmi

;;;;;;;;; BIOS START ;;;;;;;;;;;;
start:
	; clear interrupt flag right off
	ld hl,#_vdpLimi
	ld (hl),#0
	
	; clear RAM before starting
	ld hl,#0x6000			; set copy source
	ld de,#0x6001			; set copy dest
	ld bc,#0x00ff			; set bytes to copy (1 less than size)
	ld (hl),#0				; set initial value (this gets copied through)
	ldir					; do it

    ; we will have to fix the memory bank before we prepare the system RAM
	ld  sp, #0x6100			; We have using only 256 bytes for the test
	call gsinit				; Initialize global variables
	
	call _vdpinit			; Initialize VDP and sound
	
	call _main				; call the user code
	rst 0x0					; Restart when main() returns

	;; Ordering of segments for the linker - copied from sdcc crt0.s
	.area _HOME
	.area _CODE
	.area   _INITIALIZER
	.area   _GSINIT
	.area   _GSFINAL
    .area   _PADDING
        
	.area _DATA
	.area _BSS
	.area _HEAP

	.area _BSS
_vdpLimi:		; 0x80 - interrupt set, other bits used by library
	.ds 1

;;;;;;;;;;;;;; Actual NMI handler, same as my lib uses ;;;;;;;;;;;;;;;;;
    .area _CODE
run_nmi:
; all we do is set the MSB of _vdpLimi, and then check
; if the LSB is set. If so, we call user code now. if
; not, the library will deal with it when enabled.
	push af					; save flags (none affected, but play safe!)
	push hl
	
	ld hl,#_vdpLimi
	bit 0,(hl)				; check LSb (enable)
	jp z,notokay
	
; okay, full on call, save off the (other) regs
	push bc
	push de
	;push ix ; saved by callee
	push iy
	call _my_nmi			; call the lib version
	pop iy
	;pop ix
	pop de
	pop bc	
	jp clrup				

notokay:
	set 7,(hl)				; set MSb (flag)

clrup:
	pop hl					
	pop af
	retn

; void *memcpy(void *dest, const void *source, int count)
; stack has return, dest, src, count
; for LDIR, we need destination in DE, src in HL, and count in BC
_memcpy::
; be nice and save regs
    push hl
    push bc
    push de

    ld hl,#8
    add hl,sp
    ld sp,hl

    pop de
    pop hl
    pop bc

	ldir

    ld hl,#-14
    add hl,sp
    ld sp,hl

    pop de
	pop	bc
	pop	hl
	ret

	.area _GSINIT
gsinit::
    ld	bc, #l__INITIALIZER
	ld	a, b
	or	a, c                    ; size of copy in bc
	jp	z,_main         
	ld	de, #s__INITIALIZED     ; target address
	ld	hl, #s__INITIALIZER     ; source address
	ldir
	.area _GSFINAL
	ret
	
	.area _INITIALIZER

