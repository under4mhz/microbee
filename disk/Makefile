builddir=../build

all:
	cp template.dsk $(builddir)/microbee.dsk
	cp ../cpmtools-2.10/diskdefs .
	../cpmtools-2.10/cpmcp -f ds80 -T dsk $(builddir)/microbee.dsk $(builddir)/microbee.com 0:m.com
