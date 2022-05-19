# Microbee
A simple application to demonstrate how to build games for the Microbee computer

# Building and Running
A Makefile is included to build and run the demo

    sudo apt install sdcc gcc automake autoconf make mame libdsk4-dev libncurses5-dev -y
    make clean
    make

The build environment has been tested on Linux only. The same code should work on Windows with sdcc.

A patched version of cpmtools-2.10 included, since the vanilla cpmtools does not support all the possible Microbee disk formats.

For more information, see the Microbee Forum at https://microbeetechnology.com.au/forum/

