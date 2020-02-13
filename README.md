# GBEmu

GBEmu is a simple Nintendo Gameboy (DMG) emulator written in C++, using SDL for the GUI. It can boot a few ROM-only and MBC 1 games, albeit very slowly.

## Features
 - Nearly complete MBC 1 support
 - Partial GPU emulation
 - CPU stepping and breakpoints
 - Nearly complete interrupt support, including timers
 - Compiles on both Windows and Linux (and presumably MacOS as well) using CMake

## Screenshots

![Blargg's test ROM](screenshots/blarggs.png)
![Super Mario Land](screenshots/sml.png)
![Dr. Mario](screenshots/drMario.png)

## Unimplemented features/TO-DO
  - Sound support
  - HALT bug emulation
  - MBC 2 & 3 support
  - Savestates
  - Fast-forward/rewind
  - Cartridge battery RAM
  - Advanced debugging features such as memory dumping 


## Compatibility
| ROM                         | Compatibility notes                                                                                                                                                                                   |
|-----------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Asteroids                   | Boots slowly to title screen. Can get in-game, but sprites are missing/bugged.                                                                                                                        |
| Asteroids & Missile Command | Crashes on boot when trying to select the upper bits of the ROM/RAM bank (upper bit selection is unimplemented).                                                                                              |
| Donkey Kong Land            | Gets to some sort of menu (sprites are bugged), but then hangs when trying to go in-game.                                                                                                             |
| Dr. Mario                   | Boots to menus, but hangs when going in-game (probably due to incomplete timer support).                                                                                                              |
| Galaga & Galaxian           | Hangs on boot.                                                                                                                                                                                        |
| Super Mario Land            | Gets in-game and is controllable, if extremely buggy. The player sprite appears on the left side of the screen, and appears to be turned 90 degrees sideways. Scrolling appears to be broken as well. |
| Tetris                      | Gets to the title screen, but hangs on trying to get to the menus.                                                                                                                                    |
| Tetris 2                    | Hangs on the title screen.                                                                                                                                                                            |
 
