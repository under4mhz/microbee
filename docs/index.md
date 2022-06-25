In my search for platforms to support, I came across the Microbee, an Australian computer produced in the 1980s based on the z80 processor. Unusually, it supported a text display of 64 columns by default.

Below is a technical summary of the memory and ports.

# Technical Summary
## Memory Map

|Memory Range|Description|Size|
|---|---|---|
| 0000 to 3FFF | USER RAM (16K Models)| 16KB
| 4000 to 7FFF | USER RAM (32K Models)| 16KB
| 8000 to BFFF | Microworld Level II Basic | 16KB
| C000 to DFFF | Wordbee/Edasm optional ROM | 8KB
| E000 to F000 | Network/Monitor option | 4KB
| F000 to F3FF | Name Table (Memory mapped VDU) | 1KB
| F400 to F7FF | Spare RAM |1KB
| F800 to FFFF | Pattern/Colour Table (Banked) | 2KB

## Ports

| Port | Description | Direction |
|---|---|---|
| 0x02 | Sound 1 bit speaker (bit 6) | write
| 0x08 | VDU Bank Pattern/Colour at F800 (bit 6) | write
| 0x0B | Latch ROM (Enable keyboard scan) | write
| 0x0C | CRT Controller (VDU) Status | read
| 0x0C | CRT Controller (VDU) Register Set | write
| 0x0D | CRT Controller (VDU) Data Set | write

- Keyboard is accessed via CRT Controller

## Graphics
- Graphics modes
    - 64x16 (512x256 pixels)
    - 80x16 (640x256 pixels)
    - No seperate text mode
- Tile based graphics
    - 8x16 pixels each
    - 16 bytes per tile
    - 1 byte per 1 tile row (8 pixels)
    - 128 fixed tiles (ascii text)
    - 128 user definable tiles (2KB)
    - Tile Table memory mapped at 0xF000 (1K)
    - Pattern (PCG) table memory mapped at 0xF800 (2K)
- Colours
    - Fixed Palette
    - 16 colours
    - 8 bits per tile
    - 4 bit fg / 4 bit bg
    - Direct to screen
    - Independant of tiles
    - 8x16 pixels each byte
    - Single fg/bg colour per tile
    - Memory mapped and banked at 0xF800
- No hardware sprites
- Hardware cursor
- Rockwell R6545 CRT Controller

## Emulators
- Mame mbee
- uBee512 6.0
- NanoWasp (online)

## Tools
- cpmtools (custom patched version of 2.10)

## Formats

| Extension | Desciption |
|---|---|
| .bee | Standard bin, org 0x900 (for older models with rom until 0x900)
| .com | Standard bin, org 0x100 (cpm 2.2 disk based machines)
| .dsk | Double sided, 40/80 track, CPC-Emu DSK floppy disk format

## Graphics
The system has 32K available for programs/games, which is fairly large for the time (the SC-3000 came with 2K of ram).

The vdu is memory mapped and tile based. A set of characters are defined in the PCG (programmable character graphics), which are then placed on the screen as defined by the name table.

Address range F000 - F3FF is the Tile Name Table. This defines which characters are displayed on screen. This is laid out as 64x16 characters, left to right, top to bottom, making a total of 1K of ram.

Address range F800 - FFFF is the Tile Pattern Table (or the PCG, the programmable character graphics). This defines 128 possible characters to be displayed on the screen.

Address range F800 - FFFF also defines Screen Colour Table. This is banked and directly defines the colours for each cell of the entire 64x16 screen. That is, there can be different colours for the same tile in different locations. This was done so that the same font can be used, but have differently colours on the screen.

Unusually, the tiles on the microbee are 8x16 pixels in size. So while the tiles are 64x16, the screen has an impressive resolution of 512x256.

Since the screen is 64x16 tiles in size, the tiles are long and thin. This effectively makes the screen twice as wide as other systems that used 32x24 tiles. To make the tiles square, tiles will need to be 4x1tiles in size (or 32x16 pixels) per each game tile.

The system has only 128 tiles available to be defined. If each game tile is using 4 tiles each (to create 16 tiles across the screen), that allows 32 individual tiles to be defined for the game. That will require the removal of half of the animation frames, since most other systems allow 64 game tiles to be defined.

There are 128 individual (8x16) tiles that can be defined, and these can be displayed using the values 128 - 255 in the tile table. The first 0 - 127 tiles are fixed to the classic Microbee font which can’t be changed.

Typically, I’d use a custom game font for each game, which uses about 72 tiles. Since the Microbee has its own fixed font, and tiles are limited, the internal font will be used for all text. It’s not as pretty, but with the limited tile space allowable, it can’t be helped.
The tile pixels are the standard 8bits (1byte) per row each bit defining a pixel. As each tile is 16 rows high, this makes each tile 16 bytes each, laid out sequentially in PCG memory.

While the early Microbee models were monochrome, later models were all colour. The colour table uses 4bit colour, each 8bit byte defining the foreground and background colours in the first and last 4bits. The colours are laid out as per the tile table, a single byte for each character cell on the screen. This allows the same font tiles to use different colours on the screen. This does mean that for each game tile set on the screen the colour for that tile must also be set at the same time.

Since each character/tile is 8x16 pixels, a colour can only be defined every 8x16 pixels. That means each (4x1) game tile can only have 1 colour vertically, but can have four colours horizontally. This differs from the zx spectrum for example which has 2x2 game tiles, so can define two colours vertically and horizontally per tile.

Earlier colour models used a different colour encoding scheme: 5bits/3bits for foreground and background colours. I’m going to ignore these models. The earlier and later models are more common, and the games will work fine, though the colours will appear wrong.

The colour table is banked at the same location as the PCG table, and needs to be swapped to be able to write to it. This can be set by writing 0x40 or 0x00 (bit 6) to port 0x08 to set the colour and pixel tables respectively.

Early Microbee models ran 2Mhz while the later models ran at 3.5Mhz, so there is a speed difference between the colour and monochome models (the colour being faster).

## Emulators
Three emulators are available for Microbee: Mame, uBee512 and nanowasp.

### Nanowasp
Nanowasp is an online emulator for the monochrome Microbee 16K

1. Go to <a href="http://nanowasp.org/">http://nanowasp.org/</a>
2. Select Type->Upload files
3. Select a tap file.
4. Type “load” at the prompt. (Single word, by itself, no quotes)

### Mame
Mame is the classic retro hardware emulator.

Download microbee_files.zip from <a href="https://stickfreaks.com/misc/microbee">https://stickfreaks.com/misc/microbee</a>

    sudo snap install mame
    cd ~/Downloads/
    unzip microbee_files.zip
    cd /usr/share/games/mame && unzip ~/Downloads/roms/mbee.zip
    mame mbee128 -floppydisk1 "~/Downloads/disks/Adventure Games 1 (19xx)(-)[ds84].dsk"

If you have a cp/m 2.2 com file, you can also use:

    mame mbee128 -quickload1 game.com

I typically run mame with the longer command line options below. This removes the info screen, puts it into a window, and scales the window be square.

    mame mbee128  -volume -10 -window  -nounevenstretch -nofilter -nomaximize -skip_gameinfo -resolution 512x512 -intscalex 1 -intscaley 2

An issue with the mame emulation of Microbee is that it displays light grey as white. So the colours are slightly incorrect.

Below are some useful keys for mame:

|Key|Action|
|---|---|
|Insert|Enable emulation keys. Enables other keys below.
|Escape|Exit|
|Page Down|Turbo Mode (fast)
|Tab|Menu|
|F3|Reset
|F4|Show Palette
|Alt Tab|Show mouse cursor


### uBee512

uBee512 is considered the standard for emulation for Microbee in the Microbee community.

1. Download ubee512-main.zip from <a href="https://github.com/under4mhz/ubee512">https://github.com/under4mhz/ubee512</a>
2. Download 128k_premium_master_ds40.zip from the Microbee Forum respository

Unzip and build

    unzip ~/Downloads/ubee512-main.zip
    cd ubee512-main.0/src && make
    ./build/ubee512

This will leave a message about configuring ubee the first time. Run it again, display an error. Copy the appropriate roms from mame:

    cp /usr/share/games/mame/roms/mbee128p/bn56.rom  ~/.ubee512/roms/P128K.ROM
    cp /usr/share/games/mame/roms/mbee128p/bn56.rom  ~/.ubee512/roms/P512K.ROM
    cp /usr/share/games/mame/roms/mbeepc85/bas525a.rom ~/.ubee512/roms/PC85_BASIC_A.ROM
    cp /usr/share/games/mame/roms/mbeepc85/bas525b.rom ~/.ubee512/roms/PC85_BASIC_B.ROM
    cp /usr/share/games/mame/roms/mbee128p/charrom.bin ~/.ubee512/roms/
    unzip ~/Downloads/128k_premium_master_ds40.zip
    ./build/ubee512 --model=p128k -a ~/Downloads/128k_premium_master_ds40/128k_premium_master_ds40.dsk

uBee512 has a large list of other functions such creating disks and debugging the CPU. In the uBee window right click to bring up the menu. Selecting Console will bring up a window that allows commands from the command line (listed in --help) into the emulator while running. This allows the entry of debugger comands. The commands must still have the preceding double dash as per the command line.

    ./build/ubee512 --help

## Creating Media
The original Microbee models used a tape while later models supported 3 1/4in disks.

### Tapes
The original Microbee systems have BIOS rom up to until address 0x900. So for tapes, its better to have a separate crt0 that starts at address 0x900. Applications that start at 0x900 use the .bee extension.

There's a tool to create Microbee disks called bin2tap (different from the z88dk bin2tap). It's written in C# and works well with mono under Linux

<a href="https://github.com/toptensoftware/bin2tap">https://github.com/toptensoftware/bin2tap</a>

To convert a .bee file to a tap for use in an emulator.

    mono bin2tap.exe game.bee game.tap --loadaddr:0x900 --startaddr:0x900
    ubee512 --model=pc85 --conio --tapfilei=game.tap
    load

Simply type "load" at the uBee prompt in the emulator, and the game will load and run.

### Disks
Microbee disk based systems require a boot disk. Most boot disks use cp/m 2.2 as the base boot system.

There are a number of disk formats, but the typical formats are ds40 and ds80 (double sided 80 tracks). This is the format of the disk as it would appear to the machine. On top of this, there is also an encapsulation format which contains a header with extra information. Typically this is the dsk (CPC-Emu disk) format. z88dk producess edsk (extended CPC-Emu disk format), despite the command line using dsk as the option.

To create a boot disk for games, we use an existing cpm boot template and copy the programs to the disk using cpmtools. This will create boot disk that will run the game immediately on boot.

z88dk will produce a ds80 edsk, but it is a standalone non-bootable disk, which will require us to boot up on a separate bootable disk, requiring extra steps from the user to load the game, which I prefer not to do.

The applicaiton use to manipulate cpm disks is cpmtools. However vanilla cpmtools doesn't support all the Microbee disk formats. There's a patched version of cpmtools-debian-2.10-1 that includes all the Microbee formats.

To build cpmtools from source:

    sudo apt install libdsk4-dev libncurses5-dev -y
    ./configure --with-libdsk
    make
    ./cpmls -f ds80 -T dsk "~/Downloads/disk/ChickenMan\'s\ Games\ Demo\ Disc\ \(19xx\)\(ChickenMan\)\[ds80\].dsk"
    ./cpmls -f ds40 -T dsk "~/Downloads/128k_premium_master_ds40/128k_premium_master_ds40.dsk"

libDSK creates the dsk format needed by mame and uBee512 to load disks images.

The -T dsk parameter is required and specifies to use the CPC-Emu disk format. ds40 and ds80 are double side 40/80 tracks disks, and depends on the original disk format (usually specified in the file name). By default, cpmtools write raw disks (with no header information) which mame and ubee can't understand.

I've provided a boot disk template to use, so the following step is for information only.

To create a boot disk to use for applications, take ChickenMan's demo disk and remove all the files. I've found cpmrm doesn't actually work, so I need to boot into cpm using an emulator.

    cp "~/Downloads/disk/ChickenMan's Games Demo Disc (19xx)(ChickenMan)[ds80].dsk" template.dsk
    mame mbee256 -floppydisk1  "~/Downloads/256tc_system_disk_rel5_89_07_ds80/256tc_system_disk_rel5_89_07_ds80.dsk" -floppydisk2 "template.dsk"
    b:
    8
    enter

Keep ccp.sys, delete all the other files.

Use cpm tools to copy the game to the boot disk:

    cpmcp -f ds80 -T dsk game.dsk game.com 0:m.com

m.com is the menu application. This is file that is run on boot and is hardcoded into the disk image, so write the game to m.com so it auto boots.

## Compiling

I'm using sdcc for compilation. z88dk is another well recommended alternative, but for various reasons I use sdcc (mostly I came across sdcc first).

To compile a test program:

    sdasz80 -I. -g -o crt0_bee.rel crt0_bee.s
    sdcc -I. -mz80 --data-loc 0x7800 --code-loc 0x0180 --no-std-crt0 crt0_bee.rel -o game.rel -c game.c
    sdcc -I. -mz80 --data-loc 0x7800 --code-loc 0x0180 --no-std-crt0 crt0_bee.rel game.rel -o game.ihx
    objcopy --input-target=ihex --output-target=binary game.ihx game.com
    cp template.dsk game.dsk
    cpmcp -f ds80 -T dsk game.dsk game.com 0:m.com
    mame mbee128p -floppydisk1 game.dsk

Mame will boot the disk and run the demo.

## Hardware

Unlike most other platforms, there is no vblank interrupt. This is typically used for game timers. Instead, crt vblank must polled manually to determine time. To get the vblank state, read bit 6 of port 0x0C (the crt controller port).

The crt controller chip supports two graphics modes: 64x16 characters, 80x24 characters. This must be setup in the crt controller, along with the various h/v blank timing settings, which all must match. (Which will still work on emulators, but not real hardware).

To configure the crt, write the register to update to 0x0C and the value for that register into 0x0D.

There are 19 registers which control the screen resolution, scrolling and the hardware cursor.

The settings for 64x16 mode can be found in the demo source.

Sound is a 1 bit speaker on bit 6 of port 0x02. Similar to the ZX Spectrum, the speaker has to be bit banged (driven directly by the CPU) to create sound. In fact, I've used my existing ZX Spectrum sound code to drive the sound.

Keys are read via a complicated manipulation of various crt registers (18,19,16,31) and two ports (0x0B and 0x0C) to set which scan row to read and read the scan row byte.

The colour table is banked at the same location as the pattern table (0xF800) since these are both 2KB in size. This can be set by writing 0x40 or 0x00 (bit 6) to port 0x08 to set the colour and pixel tables respectively.

------

Support me on Patreon: <a href="https://www.patreon.com/Under4Mhz">https://www.patreon.com/Under4Mhz</a>
