//
// Created by andrew on 11/17/19.
//
#ifndef GBEMU_INTERPRETER_H
#define GBEMU_INTERPRETER_H

#include <string>
#include <iostream>
#include <time.h>
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

	int checkAgainst(cpuRegisterState other)
	{
		bool errored = false;
		if (this->a != other.a)
		{
			logger::logErrorNoData("Mismatch in register A!");
			std::cout << "Expected:\t0x" << std::hex << (uint16_t)this->a << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << (uint16_t)other.a << std::dec << std::endl;
			errored = true;
		}
		if (this->b != other.b)
		{
			logger::logErrorNoData("Mismatch in register B!");
			std::cout << "Expected:\t0x" << std::hex << (uint16_t)this->b << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << (uint16_t)other.b << std::dec << std::endl;
			errored = true;
		}
		if (this->c != other.c)
		{
			logger::logErrorNoData("Mismatch in register C!");
			std::cout << "Expected:\t0x" << std::hex << (uint16_t)this->c << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << (uint16_t)other.c << std::dec << std::endl;
			errored = true;
		}
		if (this->d != other.d)
		{
			logger::logErrorNoData("Mismatch in register D!");
			std::cout << "Expected:\t0x" << std::hex << (uint16_t)this->d << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << (uint16_t)other.d << std::dec << std::endl;
			errored = true;
		}
		if (this->e != other.e)
		{
			logger::logErrorNoData("Mismatch in register E!");
			std::cout << "Expected:\t0x" << std::hex << (uint16_t)this->e << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << (uint16_t)other.e << std::dec << std::endl;
			errored = true;
		}
		if (this->f != other.f)
		{
			logger::logErrorNoData("Mismatch in register F!");
			std::cout << "Expected:\t0x" << std::hex << (uint16_t)this->f << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << (uint16_t)other.f << std::dec << std::endl;
			errored = true;
		}
		if (this->h != other.h)
		{
			logger::logErrorNoData("Mismatch in register H!");
			std::cout << "Expected:\t0x" << std::hex << (uint16_t)this->h << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << (uint16_t)other.h << std::dec << std::endl;
			errored = true;
		}
		if (this->l != other.l)
		{
			logger::logErrorNoData("Mismatch in register L!");
			std::cout << "Expected:\t0x" << std::hex << (uint16_t)this->l << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << (uint16_t)other.l << std::dec << std::endl;
			errored = true;
		}

		if (this->sp != other.sp)
		{
			logger::logErrorNoData("Mismatch in register SP!");
			std::cout << "Expected:\t0x" << std::hex << this->sp << std::endl;
			std::cout << "Actual:  \t0x" << std::hex << other.sp << std::dec << std::endl;
			errored = true;
		}
		if (errored)
		{
			return -1;
		}
		return 0;
	}
	
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

extern std::string output;
extern char tempOutput;

extern bool halted;
extern bool ramEnable;
extern std::string filePath;

extern uint8_t rtcRegister;

extern time_t rawtime;
extern struct tm * timeinfo;





void writeToAddress(uint16_t, uint8_t);
uint8_t readFromAddress(uint16_t);
void handleRomWrite(uint16_t, uint8_t);
uint16_t concat(uint8_t,uint8_t);
void writePair(uint8_t&, uint8_t&, uint16_t);
void loadTestRom(std::string);
void dumpToConsole(std::string path);
void dumpRegisters();
void dumpWorkRamToFile(std::string path);
void saveState(std::string path);
void loadState(std::string path);

void handleSoundWrite(uint16_t, uint8_t);
uint8_t handleSoundRead(uint16_t address);
void handleIOWrite(uint16_t, uint8_t);
uint8_t handleIORead(uint16_t);

void initRegisters();


void inc8(uint8_t&);
void dec8(uint8_t&);
void add8(uint8_t&, uint8_t);
void add16(uint16_t&, uint16_t);
void sub8(uint8_t&, uint8_t);
void or8(uint8_t&, uint8_t);


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
