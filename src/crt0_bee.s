;;; \file crt0_bee.s
;;;
;;; \brief
;;;
;;; \copyright Copyright (c) Paul Chandler 2021
;;; All Rights Reserved
;;;

    .module crt0
    .globl    _main

    .area    _HEADER (ABS)
    ;;.org     0x900             ; early ROM based ubee have rom till 900
    .org     0x100

gsinit_start:
RAM_ADDRESS = 0x7fff
    ;; Reset vector
    di                         ; disable interrupt
    ld sp, #RAM_ADDRESS        ; Set stack pointer directly above top of memory.
    im 1                       ; interrupt mode 1 (this won't change)

clear_ram:
    ;;  for ( int i = size; i < size - 1; i++ )
    ;;       mem[i + 1] = mem[i]
    ld    bc, #l__DATA         ; data size
    dec   bc                   ; to size - 1
    ld    hl, #s__DATA         ; start
    ld    (hl), #0x00          ; clear
    ld    de, #s__DATA         ; next
    inc   de
    ldir                       ; copy i - 1 to i

gsinit_globals:

    ld    bc, #l__INITIALIZER
    ld    a, b
    or    a, c
    jr    Z, gsinit_next
    ld    de, #s__INITIALIZED
    ld    hl, #s__INITIALIZER
    ldir

gsinit_next:
    ei                        ; re-enable interrupts before going to main()
    call    _main

1$: 
    halt
    jr    1$

    .ascii 'Under4MHZ'

    ;; Ordering of segments for the linker
    .area   _HOME
    .area   _CODE
    .area   _INITIALIZER
    .area   _GSINIT
    .area   _GSFINAL
    .area   _DATA
    .area   _INITIALIZED
    .area   _BSEG
    .area   _BSS
    .area   _HEAP
    .area   _CODE
