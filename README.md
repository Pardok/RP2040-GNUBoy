# GNUBoy SDL2 Port

Port of GNUBoy for RP2040 (incl. Raspberry Pi Pico). It was developed for [Waveshare 1.3inch LCD](https://www.waveshare.com/pico-lcd-1.3.htm),
but can be ported to any device built around Pi Pico (or just RP2040 microcontroller). [PicoSystem](https://github.com/pimoroni/picosystem) for example!

# Overview 

A Multiplatform Game Boy (Color) Emulator. Free Software.

GNUBoy is a cool, old piece of software. This fork focuses on a multiplatform SDL 2 port. Hopefully this codebase can be helpful to someone in the future
or someone interested in learning about C, SDL, microcontrollers or emulator development.

Minimal requirements for you RP2040 device:
- Less then 500KB of flash for small roms, like [Petris](https://github.com/bbbbbr/Petris), which provided with the code.
- 160x144 screen, supports x1.5 scaling, so fits "perfectly"* on 240x240 screens as 240x216.
- 8 buttons – 4 for D-pad, A, B Start and Select.

*_Non-integer scaling is always not the best way to display retro games, you can read [here](https://emulation.gametechwiki.com/index.php/Scaling) why._

Fork or contribute! Based on GNUBoy and licensed under GNU GPLv2

<div align="center">
    <img alt="Blue" src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/blue.PNG">
</div>

# Releases

The latest packaged releases are available under the associated workflow. This currently is only for RP2040. For now builds provided only for 

Download Versioned Releases: (https://github.com/Pardok/RP2040-GNUBoy/releases)

Builds: (https://github.com/Pardok/RP2040-GNUBoy/actions) <br>
*Note* Builds only provided for RP2040 for each commit <br>
*Note* The stability of these builds may vary <br>

RP2040-GNUboy pre-compiled binaries will be provided with each release. The source is also provided on the release page and the binaries can be built from the source or from this repository.

For bleeding edge RP2040-GNUBoy - build the master branch. Some stable versions also have branches to build from if need be. Otherwise check the release tab for Windows binaries or build it "the unix way" for *nix / BSD / OSX / RP2040.

# Screenshots

<div align="center">

|  GB | GBC  |
|---|---|
| <img alt="Dr Mario" src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/drmario.PNG"> | <img alt="Lemmings" src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/lemmings.PNG"> |
|<img alt="Super Mario :amd" src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/supermarioland.PNG">|<img alt="Pokemon Yellow" src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/pkmnyellow.PNG">| 
|   <img alt="Tetris" src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/tetris.PNG">|<img alt="Pokemom Crystal" src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/pkmncrystal.PNG">|

</div>

# Performance

There's some hacks ported from [Super Go Play](https://github.com/mattkj/super-go-play/) implementation of [gnuboy](https://github.com/mattkj/super-go-play/tree/master/gnuboy-go/components/gnuboy). Mainly patpix removal to free up some RAM at the cost of CPU performance (which Pi Pico has plenty of).

## Accuracy

A number of cpu buxfixes were ported from [here](https://github.com/mattkj/super-go-play/tree/master/gnuboy-go/components/gnuboy)

The emulator currently fails only one CPU instruction via blarggs test rom (Windows executable running in wine on Linux):
<div align="center">
    <img src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/blargg.PNG">
</div>

## RP2040-GNUBoy v1.2.1

Improvements for v1.2.1: 

- Now compiles for Apple M1 chip.
- SDL2 audio bugfixes
- Better controller support + rumble (v1.2.0)

<div align="center">
    Pokemon Pinball (Now with rumble) <br>
    <img alt="Pinball" src="https://github.com/AlexOberhofer/SDL2-GNUBoy/raw/master/docs/img/pinball.gif">
</div>


SDL2GNUBoy has been compiled and tested on the following platforms:

- Mac (ARM)
- RP2040

# Status

RP2040 currently is in active development. See the master branch for bleeding edge RP2040-GNUBoy.

## Whats been done

Added support for RP2040, including, but not limited to:
- Video output, including 1.5x scaler
- Inputs from GPIO
- System-specific things


## Todo

- Add filesystem to support saves and savestates
- Fix issues if games won't load even with save support
- Some simple UI to save/load, change scale, show FPS, etc.
- Make project buildable for all systems (Linux, Windows, Mac) using CMake.

Maybe:
- Audio support for directly connected speaker
- Audio support for buzzer
- Support for ILI9341 screens
- Support for an SD-card (saves)

Nice-to-haves:
- Make screen layer (video.cpp) more abstract to make other screens support easier
- Loading games from an SD-card (or flash if there's plenty of it)
- Make it a module of the bigger system with UI
- Some hacky rom trimming or compression

# Configuring

Currently configuration is not working, but with filesystem support this might change.

# Configuring Boot ROM

The following bootrom files have been tested, but others may work.

|  Platform | Name | SHA1  | Notes |
|---|---|---|---|
| GB | dmg_boot.bin | 4ED31EC6B0B175BB109C0EB5FD3D193DA823339F | |
| GB | mgb_boot.bin | 4E68F9DA03C310E84C523654B9026E51F26CE7F0 | Gameboy Pocket  |
| GBC | cgb_boot.bin | 1293D68BF9643BC4F36954C1E80E38F39864528D | |
 
# Building

Currently only RP2040 builds available. 

It is pretty straightforward process on any system if you've managed to build [pico-examples](https://github.com/raspberrypi/pico-examples) before. \
I really recommend doing this first if you've never built something for the RP2040 from C/C++ sources.

### "Pico has no game"

If you want to build any other game instead of Petris, you need to replace `assets/rom.c` with dump of `.gb`/`.gbc` rom file. This method was borrowed from [RP2040-GB](https://github.com/deltabeard/RP2040-GB). To dump file you need to run:
```
xxd -i rom.gb rom.c
```
Then open it and replace the first line with:
```
const unsigned char rom_gb[] = {
```
Also scroll down to the last line and replace everything but number with:
```
const unsigned int rom_gb_len = 
```
Done! Your build will now include the rom of your selection. Also note that the Pi Pico comes with 2MB of flash memory by default. Given that the emulator itself has a non-zero size (but <500KB), you can't use any rom >1.5MB in size. Most roms are based on the physical flash size of their cartridges, so you need to look for 1MB roms. Also consider looking for US roms, which are smaller in size due to less language data. If you have a device with larger flash memory (like the PicoSystem which has 16MB!) you can use larger roms.

## Actual building

### Linux

Linux is the most straightforward way to build RP2040-GNUBoy

1. Install CMake (at least version 3.13), and GCC cross compiler
   ```sh
   sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
   ```
1. Set up Pico SDK
    - Either by cloning the SDK locally:
        - Set `PICO_SDK_PATH` to the SDK location in your environment, or pass it (`-DPICO_SDK_PATH=`) to cmake later.
    - Or with automatic download from GitHub:
        - Pass `-DPICO_SDK_FETCH_FROM_GIT=on` to cmake later.

1. Setup a CMake build directory.
      For example, if not using an IDE:
      ```sh
      $ mkdir build
      $ cd build
      $ cmake ..
      ```   
   
   When building for a board other than the Raspberry Pi Pico, you should pass `-DPICO_BOARD=board_name` to the `cmake` command above, e.g. `cmake -DPICO_BOARD=pico_w ..`
   to configure the SDK and build options accordingly for that particular board.

   Doing so sets up various compiler defines (e.g. default pin numbers for UART and other hardware) and in certain 
   cases also enables the use of additional libraries (e.g. wireless support when building for `PICO_BOARD=pico_w`) which cannot
   be built without a board which provides the requisite functionality.

   For a list of boards defined in the SDK itself, look in [this directory](src/boards/include/boards) which has a 
   header for each named board.

1. Make your target from the build directory you created.
      ```sh
      $ make
      ```

1. You now have `rp2040gnuboy.elf` to load via a debugger, or `rp2040gnuboy.uf2` that can be installed and run on your Raspberry Pi Pico via drag and drop.


# Controls (Keyboard)

Controls are as follows:

```
ESC - QUIT 
```

```
W - UP
```

```
S - DOWN
```

```
A - LEFT 
```

```
D - RIGHT 
```

```
Q - A
```

```
E - B
```

```
ENTER - START
```

```
X - SELECT
```

# Controls (Gamepad)

Controls are as follows (Based on Xbox 360 controller layout):

```
Joy/Pad Up - UP
```

```
Joy/Pad Down - DOWN
```

```
Joy/Pad Left - LEFT 
```

```
Joy/Pad Right - RIGHT 
```

```
B - A
```

```
A - B
```

```
Start - START
```

```
Back - SELECT
```

## Porting

The GNUBoy core is extremely fast and portable. Porting documentation can be found in the docs folder.

### Repository Directory
  - /lib - Library Code
  - /sys - System dependent code
  - /src - Core emulator logic
  - /include - GNUBoy Header files
  - /docs - Various docs + media from GNUBoy

## Developers

RP2040 Port: Stanislav Blindovskii

Original SDL2-GNUBoy developer: Alex Oberhofer

## Acknowledgements

[PicoSystem SDK](https://github.com/pimoroni/picosystem) and [32-blit SDK](https://github.com/32blit/32blit-sdk) that this port is heavily based on!

Petris GBC game (included as assets/rom.c): https://github.com/bbbbbr/Petris \
Also it could be seen in the releases!

See CREDITS for full list of contributors.

Thank you to the original GNUBoy developers. Please see the history branch for their versions. Legacy documentation can be found in the docs folder.
