/*
	GBEmu - A simple DMG emulator
	Copyright (C) 2020 Andrew Watson

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
//
// Created by andrew on 11/17/19.
//

#ifndef GBEMU_LCDCONTROLLER_H
#define GBEMU_LCDCONTROLLER_H



const uint32_t color0 = 0x88ffffff;
const uint32_t color1 = 0x88a8a8a8;
const uint32_t color2 = 0x88545454;
const uint32_t color3 = 0x88000000;

const uint8_t SCREEN_WIDTH = 160;
const uint8_t SCREEN_HEIGHT = 144;
extern uint32_t pixelArray[SCREEN_HEIGHT][SCREEN_WIDTH];
const uint8_t hBlankCycles = 204;//204 cycles for h-blank
const uint8_t scanlineOAMCycles = 80;//80 cycles to go through scanline while accessing OAM
const uint8_t scanlineVRAMCycles = 172;//172 cycles to go through scanline while accessing VRAM
const uint16_t vBlankCycles = 4560;//4560 cycles for vblank
const unsigned int fullFrameCycles = 70224;


const unsigned int updateFrequency = 25000; //update every 25k cycles


extern unsigned int gpuModeClock;
extern uint8_t gpuMode;
extern uint8_t line;


void initWindow();
void updateScreen(uint8_t cycleCount);

void renderScanline();
void pushBufferToWindow();

void renderTiles();
void renderSprites();


bool TestBit(unsigned int data, int position)
{
    unsigned int mask = 1<<position;
    return (data & mask) ? true : false;
}
unsigned int BitGetVal(unsigned int data, int position)
{
    unsigned int mask = 1<<position;
    return (data&mask)? 1:0;
}
unsigned int BitReset(unsigned int data, int position)
{
    unsigned int mask = 1<<position;
    data &= ~mask;
    return data;
}
unsigned int BitSet(unsigned int data, int position)
{
    unsigned int mask = 1<<position;
    data |= mask;
    return data;
}
uint32_t GetColor(uint8_t colorNum, uint16_t address);


#endif //GBEMU_LCDCONTROLLER_H
