builddir=../build

all:
	sdasz80  -I. -g -o $(builddir)/crt0_bee.rel crt0_bee.s
	sdcc -I. -mz80 --data-loc 0x7800 --code-loc 0x0180 --no-std-crt0 $(builddir)/crt0_bee.rel -o $(builddir)/microbee.rel -c microbee.c
	sdcc -I. -mz80 --data-loc 0x7800 --code-loc 0x0180 --no-std-crt0 $(builddir)/crt0_bee.rel $(builddir)/microbee.rel -o $(builddir)/microbee.ihx
	objcopy --input-target=ihex --output-target=binary $(builddir)/microbee.ihx $(builddir)/microbee.com
