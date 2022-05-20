// Copyright 2022 UnderM4hz
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Feel free to give credit

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define VDU_VSYNC_MASK ( 1 << crtStatusVSync )
#define SOUND_MASK 0x60

#define TILE_TABLE_ADDRESS 0xF000
#define PIXEL_TABLE_ADDRESS 0xF800
#define COLOUR_TABLE_ADDRESS 0xF800

// From the R6545 crt display controller manual

///< crt/vdu status byte
enum {

    crtStatusVSync = 5,                 // Video in vsync
    crtStatusLightPen,                  // Keyboard hit
    crtStatusUpdateReady,               // Wrote to reg 31
};

///< Crt registers
enum {

    crtHorizontalTotalChars = 0,
    crtHorizontalDisplayedChars = 1,
    crtHorizontalSyncPosition = 2,
    crtHorizontalVerticalSyncWidths = 3,
    crtVerticalTotalRows = 4,
    crtVerticalTotalLineAdjust = 5,
    crtVerticalDisplayedRows = 6,
    crtVerticalSyncPosition = 7,
    crtModeControl = 8,
    crtRowScanLines = 9,
    crtCursorStartLine = 10,
    crtCursorEndLine = 11,
    crtDisplayStartAddressHigh = 12,
    crtDisplayStartAddressLow = 13,
    crtCursorPositionHigh = 14,
    crtCursorPositionLow = 15,
    crtLightPenHigh = 16,
    crtLightPenLow = 17,
    crtUpdateAddressHigh = 18,
    crtUpdateAddressLow = 19,
};

static volatile __sfr __at 0x02 SoundPort;
static volatile __sfr __at 0x0C CrtRegPort;
static volatile __sfr __at 0x0D CrtDataPort;
static volatile __sfr __at 0x08 VduBankPort;

///< Set crt register value
void vdu_reg_set( unsigned char reg, unsigned char value ) {

    CrtRegPort = reg;
    CrtDataPort = value;
}

///< Setup crt to 64x16 tiles and hide the cursor
///< There's a fixed set combination of values that will work at each resolution
///< Changing individual values will often result in a blank screen on real hardware
void vdu_crt_setup() {

    vdu_reg_set( crtHorizontalTotalChars, 108 - 1 );    // horizontal sync timing constant
    vdu_reg_set( crtHorizontalDisplayedChars, 64 );     // number of displayed characters per line
    vdu_reg_set( crtHorizontalSyncPosition, 81 );       // horizontal sync position
    vdu_reg_set( crtHorizontalVerticalSyncWidths, 55 ); // horizontal and vertical sync width
    vdu_reg_set( crtVerticalTotalRows, 19 - 1 );        // vertical sync width
    vdu_reg_set( crtVerticalTotalLineAdjust, 9 );       // vertical sync timing constant
    vdu_reg_set( crtVerticalDisplayedRows, 16 );        // number of displayed rows per screen
    vdu_reg_set( crtVerticalSyncPosition, 17 );         // vertical sync position
    vdu_reg_set( crtModeControl, 72 );                  // mode control constant
    vdu_reg_set( crtRowScanLines, 16-1 );               // number of scan lines per character
    vdu_reg_set( crtCursorStartLine, 0x20+15 );         // cursor mode and start line
    vdu_reg_set( crtCursorEndLine, 15 );                // cursor end line
    vdu_reg_set( crtDisplayStartAddressHigh, 0 );       // display start address relative to 0F000h
    vdu_reg_set( crtDisplayStartAddressLow, 0 );
    vdu_reg_set( crtCursorPositionHigh, 0 );            // cursor position
    vdu_reg_set( crtCursorPositionLow, 0 );
}

///< Switch in vdu PCG or colour data at 0xF8000
void vdu_bank( int colour ) {

    VduBankPort = colour ? 0x47 : 0x07;
}

///< Clear screen
void vdu_screen_clear() {

    memset( TILE_TABLE_ADDRESS, 32, 4096 );
}

///< Setup vdu
void vdu_init() {

    vdu_screen_clear();
    vdu_crt_setup();
}

///< Poll if a key is pressed
///< Keyboard access is via the crt controller
bool is_key_down(uint8_t key) __naked __z88dk_fastcall {

   key;
__asm
    ld    a, l

    ; write the loword to register 0x12 (18)
    ld      b,a
    ld      a,#0x12
    out     (#0x0c),a
    ld      a,b
    rrca
    rrca
    rrca
    rrca
    and     #0x03
    out     (#0x0d),a

    ; write the hiword to register 0x13 (19)
    ld      a,#0x13
    out     (#0x0c),a
    ld      a,b
    rlca
    rlca
    rlca
    rlca
    out     (#0x0d),a

    ; enable latch rom (to disable key scan)
    ld      a,#0x01
    out     (#0x0b),a


    ; read register 0x10 (16) (light pen address to clear light pen flag)
    ld      a,#0x10
    out     (#0x0c),a
    in      a,(#0x0d)

    ; write to port 31 to scan the key
    ld      a,#0x1f
    out     (#0x0c),a
    out     (#0x0d),a

    ; wait for update strobe bit to be set
00001$:     in      a,(#0x0c)
    bit     #7,a
    jr      z,00001$

    ; read status register and check lpen bit
    in      a,(#0x0c)
    bit     #6,a

    ; turn off latch rom
    ld      a,#0x00
    out     (#0x0b),a

    ; setup return value
    ld      l,#1
    jr      nz,00002$
    ld      l,#0
00002$:

    ;; Clear keyboard set flag
    ld c, #0x0c
    in a, (c)

    ret
__endasm;
}

int g_seed = 1;

///< Random number
uint16_t fast_rand() __naked {

    // Super fast using xor
    // https://wikiti.brandonw.net/index.php?title=Z80_Routines:Math:Random
    __asm

    ld hl,(_g_seed)       ; seed must not be 0

    ld a,h
    rra
    ld a,l
    rra
    xor h
    ld h,a
    ld a,l
    rra
    ld a,h
    rra
    xor l
    ld l,a
    xor h
    ld h,a

    ld (_g_seed),hl

    ;; for sdcc 4.2 abi
    ld d,h
    ld e,l

    ret

    __endasm;

    // Suppress warning, returns hl above
    return 0;
}

///< Random stuff of on screen
void display_test() {
    
    uint8_t *ptr = TILE_TABLE_ADDRESS;
    uint16_t size = 0x400;

    // User defined tiles are at 128-255
    for( int i = 0; i < size / 2; i++ )
        *ptr++ = ( fast_rand() % 64 ) + 128;
    // Fixed ascii tiles are at 0-127
    for( int i = 0; i < size / 2; i++ )
        *ptr++ = ( fast_rand() % 64 );

    // Pixel pattern data
    // Pixel data applied to the tiles,
    // which are set in the tile table

    // Use pixel memory bank
    vdu_bank(0);

    // Random pixels
    ptr = PIXEL_TABLE_ADDRESS;
    size = 0x800;
    for( int i = 0; i < size; i++ )
        *ptr++ = fast_rand();

    // Colour data
    // Colour data is set directly to the screen, and does not effect tiles

    // Use colour memory bank
    vdu_bank(1);

    // Random colours
    ptr = PIXEL_TABLE_ADDRESS;
    size = 0x800;
    for( int i = 0; i < size; i++ )
        *ptr++ = fast_rand();
}

///< Methods of producing 1 bit sound
void sound_test() {

    // Pulse width modulation (PWM) (10% on / 90% off)
    // Using shorter pulses, but at a set wave length,
    // it's possible to have multiple channels, but gives a buzzing quality
    for( int i = 0; i < 200; i++ ) {

        SoundPort = SOUND_MASK;
        for( int delay = 0; delay < 2; delay++ );
        SoundPort = 0;
        for( int delay = 0; delay < 200; delay++ );
    }

    // Pure (50% on / 50% off).
    // A clear sound, but multiple channels is difficult.
    for( int i = 0; i < 200; i++ ) {

        SoundPort = SOUND_MASK;
        for( int delay = 0; delay < 50; delay++ );
        SoundPort = 0;
        for( int delay = 0; delay < 50; delay++ );
    }

    // Noise. For explosions and shooting and the like.
    for( int i = 0; i < 3000; i++ ) {

        SoundPort = fast_rand() & SOUND_MASK;
    }
}

void keyboard_test() {

    char keys[32] = "You Pressed  ";

    // Punctuation tests as lower case

    for( int key = 'A'; key < 'z'; key++ ) {
        
        if ( is_key_down( key ) ) {

            keys[12] = key;
            memcpy( TILE_TABLE_ADDRESS + 216, keys, strlen( keys ) );
            vdu_bank(1);
            memset( PIXEL_TABLE_ADDRESS + 216, 0x0f, strlen( keys ) );
            break;
        }
    }
}

void main() {

    vdu_init();

    for(;;) {

        display_test();

        keyboard_test();

        sound_test();
    }
}
