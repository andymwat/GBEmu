# GBEmu

GBEmu is a simple Nintendo Gameboy (DMG) emulator written in C++, using SDL for the GUI. It can run a large portion of the Game Boy library, including popular titles such as Pokemon and Kirby.

## Building

To build from source, clone the repository and `cd` into the directory. From there, run the following command to build the Release target:
```
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build/Release/ && cd build/Release/ && make
```
To build the Debug target, run:
```
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build/Debug/ && cd build/Debug/ && make
```
Run the `GBEmu` executable in the corresponding directory to run the emulator.

## Controls
 - Start: A
 - Select: S
 - A: Z
 - B: X
 - D-pad: Arrows
 - Save current cartridge RAM: P
 - Manually load cartridge RAM: L
 - Fast-forward: Space
 - Increase/Decrease volume: [ and ]

## Features
 - Passes all tests in blargg's cpu_instrs test ROM
 - Nearly complete MBC 1 and 3 support
 - Complete (if buggy) sound support
 - Real Time Clock (RTC) support
 - CPU stepping and breakpoints
 - Nearly complete interrupt support, including timers
 - Compiles on both Windows and Linux (and presumably MacOS as well) using CMake

## Screenshots

![Blargg's test ROM](screenshots/blarggs.png)
![Kirby](screenshots/kirby.png)
![Zelda](screenshots/zelda.png)
![Pokemon](screenshots/pkmn.png)
![Super Mario Land](screenshots/sml.png)
![Dr. Mario](screenshots/drMario.png)
![Tetris](screenshots/tetris.png)
![Asteroids](screenshots/asteroids.png)
![Galaga](screenshots/galaga.png)

## Unimplemented features/TO-DO
  - Fix sound bugs
  - HALT bug emulation
  - MBC 2 support
  - Savestates
  - Rewind
  - Advanced debugging features such as memory dumping 
  - Screen upscaling/filters
  - UI


## Compatibility
| ROM                         | Compatibility notes                                                                                                                                                                                   |
|-----------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Asteroids                   | Fully playable, but the ship sprite occasionally bugs out.                                                                                                                     |
| Asteroids & Missile Command | Fully playable. 
| Blargg's Test ROMs          | Passes all tests in the cpu_instrs ROM.
| Donkey Kong Land            | Title screen shows, and gets to a flickering menu after that. Can get in-game, but video is messed up. I think the cartridge assumes it's running on a GBC.                                                                                                             |
| Dr. Mario                   | Boots to menus, but hangs when going in-game (probably due to incomplete timer support).                                                                                                              |
| Galaga & Galaxian           | Fully playable.                                                                                                                                                                                     |
| Kirby's Dream Land          | Playable, but sound is buggy.			 |
| Pokemon Gold				  | Playable, but sound is buggy.  |
| Pokemon Red				  | Fully playable, but sound is kinda buggy. |
| Pokemon Yellow			  | Intro plays, but crashes on an unimplemented ROM write (MBC5) |
| Super Mario Land            | Fully playable.			 |
| The Legend of Zelda: Link's Awakening| Fully playable. |
| Tetris                      | Fully playable.                                                                                                                                    |
| Tetris 2                    | Fully playable.                                                                                                                                                                            |
 
## Known Issues
  - Sound is buggy, and can sometime desync with the video. It is very dependent upon CPU speed. Faster systems will cause the sound to play faster and at a higher pitch. This also seems to affect Windows more than Linux, possibly due to the higher precision of usleep vs. SDL_Delay.
  - Incomplete timer support causes some games to hang.
  - Not all MBC configurations are supported.

## Misc.

This software uses SDL, which is [licensed](https://www.libsdl.org/license.php) under the [zlib license](https://www.zlib.net/zlib_license.html).
