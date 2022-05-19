all: init cpmtools demo disk

builddir=../build/
mameargs=-volume -25 -window  -nounevenstretch -nofilter -nomaximize -skip_gameinfo -resolution 512x512 -intscalex 1 -intscaley 2

init:
	-mkdir build

demo:
	cd src && make

disk:
	cd disk && make
	mame mbee128p $(mameargs) -floppydisk1 build/microbee.dsk
	#mame -debug mbee128p $(mameargs) -floppydisk1 build/microbee.dsk

cpmtools:
	-cd cpmtools-2.10 && test ! -e Makefile && ./configure --with-libdsk
	cd cpmtools-2.10 && make

.PHONY: disk demo cpmtools init

clean:
	rm -rf build
	-cd cpmtools-2.10 && test -e Makefile && make distclean
