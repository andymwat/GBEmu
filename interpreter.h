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
#ifndef GBEMU_INTERPRETER_H
#define GBEMU_INTERPRETER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

#include "cartridge.h"
#include "logger.h"

extern uint8_t a,b,c,d,e,f,h,l;
extern uint16_t sp;
extern uint16_t pc;
extern bool runningTest;
extern uint8_t* testInstructions;

struct cpuRegisterState
{
	uint8_t a, b, c, d, e, f, h, l;
	uint16_t sp;
	uint16_t pc;
};


extern uint8_t vram[8192];
extern uint8_t* cartRam;
extern uint8_t workRam[8192];
extern uint8_t* rom0;//first 256 are internal bootROM
extern uint8_t* romSwitchable;
extern uint8_t backgroundPalette, scrollX, scrollY, windowX, windowY, lcdControl, lcdStatus, coincidence, bgPaletteData, objectPalette0Data, objectPalette1Data;
extern uint8_t highRam[127];

extern bool enableInterrupts;
extern bool lcdEnable, windowDisplayEnable, spriteDisplayEnable, bgDisplayEnable;
extern bool spriteSize, bgTilemapDisplaySelect, bgAndWindowTileDataSelect, windowTilemapDisplaySelect;
extern uint8_t interruptRegister, interruptFlag;

extern uint8_t m_DividerRegister;
extern unsigned int m_DividerCounter;
extern uint8_t serialControl;

extern uint8_t joypadRegister;

extern uint8_t tima,tma,tmc;


extern unsigned int cycles;
extern double cycleTime;
extern unsigned int clockSpeed;
extern int m_TimerCounter;

extern int errorAddress;
extern cartridge* currentCartridge;

extern const char* output;
extern char tempOutput;

extern bool halted;
extern bool ramEnable;
extern const char* filePath;

extern uint8_t rtcRegister;

extern time_t rawtime;
extern struct tm * timeinfo;

void writeToAddress(uint16_t, uint8_t);
uint8_t readFromAddress(uint16_t);
void handleRomWrite(uint16_t, uint8_t);
uint16_t concat(uint8_t,uint8_t);
void writePair(uint8_t*, uint8_t*, uint16_t);

void loadTestRom(const char*);
void dumpToConsole(const char* path);
void dumpRegisters();
void dumpWorkRamToFile(const char* path);
void saveState(const char* path);
void loadState(const char* path);

void handleSoundWrite(uint16_t, uint8_t);
uint8_t handleSoundRead(uint16_t address);
void handleIOWrite(uint16_t, uint8_t);
uint8_t handleIORead(uint16_t);

void initRegisters();


void inc8(uint8_t*);
void dec8(uint8_t*);
void add8(uint8_t*, uint8_t);
void add16(uint16_t*, uint16_t);
void sub8(uint8_t*, uint8_t);
void or8(uint8_t*, uint8_t);


bool carryStatus();
void setCarry(bool);
bool halfStatus();
void setHalf(bool);
bool subtractStatus();
void setSubtract(bool);
bool zeroStatus();
void setZero(bool);

void switchBank(uint8_t number);
void switchRamBank(uint8_t number);

void checkInterrupts();
void processTimer(uint8_t opCycle);
void doDividerRegister(uint8_t opCycle);
uint8_t GetClockFreq();
void SetClockFreq();
void doDMATransfer(uint8_t);

void executePrefixedInstruction(uint8_t opcode);

#endif //GBEMU_INTERPRETER_H
