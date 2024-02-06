# scGBe

**s**imple **c**pp **G**ame**B**oy **e**mulator is, as the name describes, a basic Game Boy emulator, written in C++ as a personal challenge.

It's not meant to be 100% accurate to the original hardware, but it still passes most basic test ROMs, such as [dmg-acid2](https://github.com/mattcurrie/dmg-acid2), [blargg's cpu-instrs](https://github.com/retrio/gb-test-roms), and most [numism](https://github.com/pinobatch/numism) tests.

## Acknowledgements
* [GBDev's Pandocs](https://gbdev.io/pandocs/) for being a great reference for almost every every aspect of the GB's design
* [Gekkio's Game Boy: Complete Technical Reference](https://gekkio.fi/files/gb-docs/gbctr.pdf) and [RGBDS' gbz80 CPU opcode reference doc](https://rgbds.gbdev.io/docs/v0.7.0/gbz80.7) for their SM83 processor opcode tables and descriptions

## Building

scGBe uses CMake with MinGW Makefiles for building to Windows. GNU/G++ must be used as the compiler for its [switch range extension](https://gcc.gnu.org/onlinedocs/gcc/Case-Ranges.html).

### Dependencies
* [SFML](https://www.sfml-dev.org/) will be automatically installed by CMake. Used for the game window, input, and audio streaming.

### Windows Building Guide
1. Download and install [MSYS2](https://www.msys2.org/).
2. Open "MSYS2 UCRT64" to bring up a bash terminal. Should be searchable from the start menu.
3. Run `pacman -S mingw-w64-x86_64-gcc` in this terminal to install the latest version of the GCC/G++ compiler.
4. [Create an `%mingw64%` environment variable](https://phoenixnap.com/kb/windows-set-environment-variable), pointing at your MinGW folder. *(By default, it should be at `C:\msys64\mingw64`)
5. Download and install [CMake](https://cmake.org/download/).
6. Clone the project (or download as ZIP + extract).
7. Run `cmake -G "MinGW Makefiles"` within the root directory of the project. *(This only has to be done once, and can be done using any terminal, not just MSYS2's.)*
8. Next, run `cmake --build .` within the root directory of the project to build to an executable. The final executable will be placed at `/bin/scGBe.exe` within the project folder. *(This can be done using any terminal, not just MSYS2's.)*