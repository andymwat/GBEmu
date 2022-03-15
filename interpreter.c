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

#include "cartridge.h"
#include "interpreter.h"
#include "lcdController.h"
#include "keyboardInput.h"
#include "main.h"
#include "logger.h"
#include "audioController.h"

//warn about unimplemented features
#define LOG_VERBOSE false


time_t rawtime;
struct tm * timeinfo;

uint8_t rtcRegister = 0x00;
const char* filePath;
int errorAddress = -1;
const char* output = "Program output: \n";
bool enableInterrupts = true;
bool lcdEnable = true;
bool windowDisplayEnable = true;
bool spriteDisplayEnable = false;
bool bgDisplayEnable = true;
bool spriteSize = false;
bool bgTilemapDisplaySelect = false;
bool windowTilemapDisplaySelect = false;
bool bgAndWindowTileDataSelect = true;
bool ramEnable;
unsigned int cycles = 0;
double cycleTime = 0.23841880647;//microseconds
unsigned int clockSpeed = 4194304;//4.2 mhz
int m_TimerCounter;
uint8_t a, b, c, d, e, f, h, l;
uint16_t sp;
uint16_t pc;
bool runningTest = false;
uint8_t* testInstructions;

uint8_t vram[8192];
uint8_t* cartRam;
uint8_t workRam[8192];
uint8_t* rom0;//first 256 are internal bootROM
uint8_t* romSwitchable;
uint8_t backgroundPalette, scrollX, scrollY, windowX, windowY, lcdControl, coincidence, lcdStatus, objectPalette0Data, objectPalette1Data;;
uint8_t highRam[127];
uint8_t spriteAttributeTable[160];

uint8_t joypadRegister = 0xf;
uint8_t interruptRegister = 0;
uint8_t interruptFlag = 0;

uint8_t m_DividerRegister = 0;
unsigned int m_DividerCounter;
uint8_t serialControl;
uint8_t tima, tma, tmc;
cartridge* currentCartridge;
char tempOutput;
bool halted = false;
bool ROM_RAM_Mode = false;
uint8_t romUpperBits = 0x00;



/*
 * Memory map:
 * 0x0000-0x3FFF: ROM Bank 0 (16k)
 * 0x4000-0x7FFF: Switchable ROM bank (16k)
 * 0x8000-0x9FFF: Video RAM (8k)
 * 0xA000-0xBFFF: Switchable RAM AKA .sav (8k)
 * 0xC000-0xCFFF: Work RAM bank 0 (4k)
 * 0xD000-0xDFFF: Work RAM bank 1 (4k)
 * 0xE000-0xFDFF: Echo of work RAM (8k)
 * 0xFE00-0xFE9F: Sprite Attribute table (160)
 * 0xFEA0-0xFEFF: Not mapped
 * 0xFF00-0xFF7F: Device mappings (128)
 * 0xFF80-0xFFFE: High RAM (127)
 * 0xFFFF-0xFFFF: Interrupt enable
 */

void writeToAddress(uint16_t address, uint8_t data) {

	// std::cout<<"Writing address " << std::to_string(address) <<std::endl;
	if (address >= 0x8000 && address <= 0x9fff)
	{
		vram[address - 0x8000] = data;
	}
	else if (address == 0xff7f)
	{
		if (LOG_VERBOSE)
			logInfo("Wrote to 0xff7f, probably a ROM bug.");
	}
	else if (address >= 0xc000 && address <= 0xdfff)
	{
		workRam[address - 0xc000] = data;
	}
	else if (address >= 0xe000 && address <= 0xfdff)//mirror
	{
		workRam[address - 0xe000] = data;
	}
	else if (address <= 0x7FFF)//write to ROM
	{
		handleRomWrite(address, data);
	}
	else if (address >= 0xff10 && address <= 0xff3f)//sound
	{
		handleSoundWrite(address, data);
	}
	else if (address >= 0xa000 && address <= 0xbfff)
	{
		if (ramEnable)
		{
			if (currentCartridge->ramBankIdentifier == 0x00)//no cartRAM
			{
				logWarningNoData(" Attempting to write to cartRAM on cartridge without RAM, ignoring.");

			}
			else {
				if (rtcRegister < 0x08  || rtcRegister > 0x0c)//should never be greater
				{
					cartRam[address - 0xa000] = data;
				}
				else
				{
					if (LOG_VERBOSE)
						logWarning("RTC write ignored.", address, data);
				}
			}
		}
		else
		{
			//logWarningNoData(" Attempting to write to cartRAM while RAM is disabled, ignoring.");
		}
		//cout<<"WARNING: Cart RAM banking is not tested. Writing to address 0x" <<hex<<address<<dec<<".\n";

	}
	else if (address >= 0xff00 && address <= 0xff7f)
	{
		handleIOWrite(address, data);
	}
	else if (address >= 0xff80 && address <= 0xfffe)
	{
		highRam[address - 0xff80] = data;
	}
	else if (address == 0xffff)//interrupts
	{
		interruptRegister = data;
	}
	else if (address >= 0xfea0 && address <= 0xfeff)
	{
		if (LOG_VERBOSE)
			logInfo("Tried to write to an unused address, probably a ROM bug.");
	}
	else if (address >= 0xfe00 && address <= 0xfe9f)
	{
		//cout<<"Writing to sprite attribute table address "<<to_string(0xfe00-address)<<endl;
		spriteAttributeTable[address - 0xfe00] = data;
	}
	else {
		errorAddress = address;
		logError("WRITE ADDRESS NOT IMPLEMENTED", address, data);
	}

}
uint8_t readFromAddress(uint16_t address) {
	// std::cout<<"Reading address "  << std::to_string(address) <<std::endl;
	if (address <= 0x3fff)
	{
		//  std::cout << "Read from ROM address 0x"<<hex<<address<<dec <<", value is 0x" <<hex<<(uint16_t)rom0[address] <<dec<<std::endl;
		return rom0[address];
	}
	if (address <= 0x7fff && address >= 0x4000)
	{
		//cout<<"reading from romSwitchable address "<<to_string(address)<<endl;
		return romSwitchable[address - 0x4000];
	}
	else if (address >= 0x8000 && address <= 0x9fff)
	{
		return vram[address - 0x8000];
	}
	else if (address >= 0xc000 && address <= 0xdfff)
	{
		return workRam[address - 0xc000];
	}
	else if (address >= 0xe000 && address <= 0xfdff)//mirror
	{
		return workRam[address - 0xe000];
	}
	else if (address >= 0xa000 && address <= 0xbfff)
	{
		if (ramEnable)
		{
			if (currentCartridge->ramBankIdentifier == 0x00)//no cartRAM
			{
				logWarning("Attempting to read from cartRAM on cartridge without RAM, returning 0xff.", address, 0xff);
				return 0xff;
			}
			else {
				if (rtcRegister < 0x08 || rtcRegister > 0x0c)//should never be greater
				{
					return cartRam[address - 0xa000];
				}
				else //rtc read
				{
					time(&rawtime);
					timeinfo = localtime(&rawtime);
					switch (rtcRegister)
					{
					case 0x08://seconds
						return timeinfo->tm_sec;
						break;
					case 0x09:
						return timeinfo->tm_min;
						break;
					case 0x0a:
						return timeinfo->tm_hour;
						break;
					case 0x0b:
						return (timeinfo->tm_yday & 0xff);
						break;
					case 0x0c:
						return (timeinfo->tm_yday & 0x0100) >> 8;
						break;
					default:
					    logErrorNoData("RTC error!");
                        exit(-1);
					}
				}
			}
		}
		else
		{
			logWarning("Attempting to read from cartRAM while RAM is disabled, returning 0xff.", address, 0xff);
			return 0xff;
		}
	}
	else if (address >= 0xff80 && address <= 0xfffe)
	{
		return highRam[address - 0xff80];
	}
	else if (address == 0xffff)//interrupts
	{
		return interruptRegister;
	}
	else if (address >= 0xff10 && address <= 0xff3f)//sound
	{
		return handleSoundRead(address);
	}
	else if ((address >= 0xff00 && address < 0xff10) || (address > 0xff3f && address <= 0xff7f))
	{
		return handleIORead(address);
	}
	else if (address >= 0xfea0 && address <= 0xfeff)
	{
		if (LOG_VERBOSE)
			logWarning("Tried to read from to an unused address, probably a ROM bug, returning 0xff.", address, 0xff);
		return 0xff;
	}
	else if (address >= 0xfe00 && address <= 0xfe9f)
	{
		return spriteAttributeTable[address - 0xfe00 ];
	}
	else {
		errorAddress = address;
		logError("Read address not imprlemented, returning 0xff.", address, 0xff);
		return 0xff;
	}
}


void handleRomWrite(uint16_t address, uint8_t data)
{
	if (currentCartridge->mbcType == 0x00)//ROM only, 2 banks
	{
		logWarning("Trying to write to ROM of cart without MBC", address, data);
	}
	else if (currentCartridge->mbcType == 0x01)//mbc1
	{
		if (address >= 0x2000 && address <= 0x3fff)//bank select
		{
			if (data == 0 || data == 0x20 || data == 0x40 || data == 0x60)
			{
				//logWarning("Wrote 0x00, 0x20, 0x40, or 0x60 to bank select, translating to 0xX1.", address, data);
				switchBank(data+1); //bank 0x00 translates to 0x01, 0x20 to 0x21, etc
			}
			else
			{
				switchBank(data | (romUpperBits << 5));
			}
			//cout<<"DEBUG: Selected ROM bank "<<to_string(data) <<hex<<"\t0x"<<(uint16_t)(data)<<dec<<endl;
			//usleep(500000);
		}
		else if (address <= 0x1fff)//ram enable
		{
			//cout << "WARNING: RAM enable and banking is not tested.\n";
			if ((data & 0x0f) == 0x0a)
			{
				if (LOG_VERBOSE)
					logInfo("CartRAM is enabled.");
				ramEnable = true;
			}
			else
			{
				if (LOG_VERBOSE)
					logInfo("CartRAM is disabled.");
				ramEnable = false;
			}
		}
		else if (address >= 0x4000 && address <= 0x5fff)
		{
			romUpperBits = data & 0x3;
			//throw "Wrote to unimplemented RAM bank/upper bits of ROM bank";
		}
		else if (address >= 0x6000 && address <= 0x7fff)
		{
			if (data != 0)
			{
				logError("Error selecting RAM banking mode, unimplemented.", address, data);
                exit(-1);
			}
			if (LOG_VERBOSE)
				logInfo("Now using upper bits of ROM bank.");
			ROM_RAM_Mode = false;
		}
		else
		{
			logError("Error writing to ROM address.", address, data);
			errorAddress = address;
            exit(-1);
		}
	}
	else if (currentCartridge->mbcType == 0x03)//mbc1 + RAM + Battery
	{
		if (address >= 0x2000 && address <= 0x3fff)//bank select
		{
			switchBank(data);
		}
		else if (address <= 0x1fff)//ram enable
		{
            ramEnable = (data & 0x0f) == 0x0a;
		}
		
		else
		{
			logError("Error writing to ROM address.", address, data);
			errorAddress = address;
            exit(-1);
		}
	}
	else if (currentCartridge->mbcType == 0x13 || currentCartridge->mbcType == 0x10) //mbc3 + RAM + battery (timer unimplemented)
	{
		
		if (address <= 0x1fff)//RAM and Timer enable
		{
            ramEnable = (data & 0x0f) == 0x0a;
		}
		else if (address >= 0x2000 && address <= 0x3fff)
		{
			if (data == 0)
			{
				switchBank(1);
			}
			else
			{
				switchBank(data & 0x7f); //7 bit bank value
			}
		}
		else if (address >= 0x4000 && address <= 0x5fff)
		{
			if (data >= 0x00 && data <= 0x03)//select RAM bank
			{
				rtcRegister = data;
				switchRamBank(data);
			}
			else if (data >= 0x08 && data <= 0x0c)//switch to RTC register
			{
				rtcRegister = data;
			}
			else
			{
				logError("Writing invalid data to RAM bank/RTC select!", address, data);
			}
		}
	}
	else if (currentCartridge->mbcType == 0x1b) //MBC5 + RAM + Battery
	{
		if (address >= 0x2000 && address <= 0x2fff)//bank select low 8 bits
		{
			switchBank(data);
		}
		else if (address <= 0x1fff)//ram enable
		{
            ramEnable = (data & 0x0f) == 0x0a;
		}
		else
		{
			logError("Error writing to ROM address.", address, data);
			errorAddress = address;
            exit(-1);
		}
	}
	
	else
	{
		logError("Error writing to ROM address.", address, data);
		errorAddress = address;
		exit(-1);
	}
}

uint16_t concat(uint8_t high, uint8_t low) {
	return high * 256 + low;
}

void writePair(uint8_t *x, uint8_t *y, uint16_t data) {
	//cout<<"data: " <<to_string(data) <<endl<<"x before: " <<to_string(x)<<endl;
	*x = ((data & 0xff00) >> 8);
	//cout<<" and after: " <<to_string(x)<<endl;
	//cout<<"y before: " <<to_string(y)<<endl;
	*y = data & 0x00ff;
	//cout<<"and after " <<to_string(y)<<endl;
	//cout<<"concat after: " <<to_string(concat(x,y))<<endl;
}

void handleSoundWrite(uint16_t address, uint8_t data)
{
	writeToAudioRegister(address, data);
	//logWarning("Sound registers write unimplemented, ignoring.", address, data);
}
uint8_t handleSoundRead(uint16_t address)
{
	//logWarning("Sound registers read unimplemented, returning 0x0", address, 0x0);
	return readFromAudioRegister(address);
}

void handleIOWrite(uint16_t address, uint8_t data) {
	if (address == 0xff4d)
	{
		//logWarning("Speed control register write unimplemented.", address, data);
	}
	else if (address == 0xff00)
	{
		//logWarning("Joypad Write", pc, joypadRegister);
		joypadRegister = data & 0xf0;
	}
	else if (address == 0xff42)
	{
		scrollY = data;
	}
	else if (address == 0xff43)
	{
		scrollX = data;
	}
	else if (address == 0xff44)
	{
		line = 0;
	}
	else if (address == 0xff45)
	{
		coincidence = data;
	}
	else if (address == 0xff46)//dma transfer
	{
		doDMATransfer(data);
	}
	else if (address == 0xff47)
	{
		backgroundPalette = data;
	}
	else if (address == 0xff4a)
	{
		windowY = data;
	}
	else if (address == 0xff4b)
	{
		windowX = data-7;
	}
	else if (address == 0xff4f)
	{
		if (LOG_VERBOSE)
			logWarningNoData("VRAM bank registers write unimplemented.");
		//CGB only
	}
	else if (address == 0xff40)
	{
		lcdControl = data;
		bgDisplayEnable = (lcdControl & 0x01) == 0x01;//test for bg display enable bit
		spriteDisplayEnable = (lcdControl & 0x02) == 0x02;//...
		spriteSize = (lcdControl & 0x04) == 0x04;//... 1 = 8x16, 0 = 8x8
		bgTilemapDisplaySelect = (lcdControl & 0x08) == 0x08;//...
		bgAndWindowTileDataSelect = (lcdControl & 0x10) == 0x10;//...
		windowDisplayEnable = (lcdControl & 0x20) == 0x20;//...
		windowTilemapDisplaySelect = (lcdControl & 0x40) == 0x40;//...
		lcdEnable = (lcdControl & 0x80) == 0x80;//test for lcd enable bit
	}
	else if (address == 0xff05)
	{
		tima = data;
	}
	else if (address == 0xff06)
	{
		tma = data;
	}
	else if (address == 0xff07)//tmc
	{
		uint8_t currentFreq = GetClockFreq();
		tmc = data;
		uint8_t newFreq = GetClockFreq();
		if (currentFreq != newFreq)
		{
			SetClockFreq();
		}
		if (LOG_VERBOSE)
		{
			if (TestBit(tmc, 2))
			{
				logInfo("Timer is enabled.");
			}
			else
			{
				logInfo("Timer is disabled.");
			}
		}
	}
	else if (address == 0xff04)
	{
		m_DividerRegister = 0;
	}
	else if (address == 0xff0f)
	{
		interruptFlag = data;
	}
	else if (address == 0xff01)//serial data (SB)
	{
		tempOutput = (char)data;
	}
	else if (address == 0xff02)//serial control (SC)
	{
		serialControl = data;
		if (data == 0x81)
		{
			output += (char)tempOutput;
		}
		if (LOG_VERBOSE)
		{			logWarning("Serial control write unimplemented.", address, data);
		}
	}
	else if (address == 0xff41)
	{
		lcdStatus = data;
	}
	else if (address == 0xff48)
	{
		objectPalette0Data = data;
	}
	else if (address == 0xff49)
	{
		objectPalette1Data = data;
	}
	else if (address == 0xff56)
	{
		if (LOG_VERBOSE)
			logWarningNoData("Tried to write infrared port, ignoring.");
	}
	else if (address == 0xff4f)
	{
		if (LOG_VERBOSE)
			logWarningNoData("Tried to write CGB VRAM bank, ignoring.");
	}
	else
	{
		errorAddress = address;
		logError("IO WRITE NOT IMPLEMENTED", address, data);
	}
}
uint8_t handleIORead(uint16_t address) {
	if (address == 0xff00)
	{
		// return getJoypadState();
		 //logWarning("Joypad Read", pc, joypadRegister);
		return joypadRegister;
	}
	else if (address == 0xff02)
	{	
		if (LOG_VERBOSE)
			logWarningNoData("Serial control register unimplemented, returning 0xff.");
		return 0xff;
	}
	else if (address == 0xff01)
	{
		return tempOutput;
	}
	
	else if (address == 0xff40)
	{
		return lcdControl;
	}
	else if (address == 0xff41)
	{
		return lcdStatus;
	}
	else if (address == 0xff44)//lcdc y coordinate (0-153, 144-153 is vblank)
	{
		return line;
	}
	else if (address == 0xff45)
	{
		return coincidence;
	}
	else if (address == 0xff47)
	{
		return backgroundPalette;
	}
	else if (address == 0xff4a)
	{
		return windowY;
	}
	else if (address == 0xff4b)
	{
		return windowX+7;
	}
	else if (address == 0xff4d)//cgb key1 (prepare speed switch)
	{
		if (LOG_VERBOSE)
			logWarning("Speed control registers read unimplemented, returning 0xff", address, 0xff);
		return 0xff;
	}
	else if (address == 0xff4f)
	{
		if (LOG_VERBOSE)
			logWarning("VRAM bank registers read unimplemented, returning 0xff", address, 0xff);
		return 0xff;
		//CGB only
	}
	else if (address == 0xff0f)
	{
		return interruptFlag | 0xe0;//set top 3 bits to always be true
	}
	else if (address == 0xff48)
	{
		return objectPalette0Data;
	}
	else if (address == 0xff49)
	{
		return objectPalette1Data;
	}
	else if (address == 0xff04)
	{
		return m_DividerRegister;
	}
	else if (address == 0xff05)
	{
		return tima;
	}
	else if (address == 0xff06)
	{
		return tma;
	}
	else if (address == 0xff07)
	{
		return tmc;
	}
	else if (address == 0xff56)
	{
		if (LOG_VERBOSE)
			logWarningNoData("Tried to read infrared port, returning 0xff.");
		return 0xff;
	}
	else
	{
		errorAddress = address;
		logError("IO READ NOT IMPLEMENTED, returning 0xff", address, 0xff);
		return 0xff;
	}
}

void loadTestRom(const char* path)//calculates number of banks based on cartridge MBC number
{
	filePath = path;
	logInfo("Loading test ROM. Path:");
	logInfo(path);
	FILE* infile = fopen(path, "r");
	if (infile != NULL)
	{
		logErrorNoData("ROM file not found!");
		exit(-1);
	}
	uint8_t mbc, banks, ramBanks;


	fseek(infile, 0, SEEK_END);
	size_t length = ftell(infile);
	fseek(infile, 0x0147, SEEK_SET);
	fread(&mbc, 1, 1, infile);
	fread(&banks, 1, 1, infile);
	fread(&ramBanks, 1, 1, infile);

	currentCartridge = newCartridge(mbc, banks, ramBanks);
    fseek(infile, 0, SEEK_SET);
	fread(currentCartridge->banks, length, 1, infile);
	rom0 = currentCartridge->banks;//pointer to beginning of banks
	cartRam = currentCartridge->ramBanks;
	romSwitchable = (uint8_t*)(currentCartridge->banks + bankSize);//romSwitchable points to 2nd bank at first
}

bool carryStatus()
{
	return (f & 0x10) == 0x10;
}
bool halfStatus()
{
	return (f & 0x20) == 0x20;
}
bool subtractStatus()
{
	return (f & 0x40) == 0x40;
}
bool zeroStatus()
{
	return (f & 0x80) == 0x80;
}
void setCarry(bool value)
{
	if (value)
	{
		f |= 0x10;
	}
	else {
		f &= 0xef;
	}
}
void setHalf(bool value)
{
	if (value)
	{
		f |= 0x20;
	}
	else {
		f &= 0xdf;
	}
}
void setSubtract(bool value)
{
	if (value)
	{
		f |= 0x40;
	}
	else {
		f &= 0xbf;
	}
}
void setZero(bool value)
{
	if (value)
	{
		f |= 0x80;
	}
	else {
		f &= 0x7f;
	}
}

void dumpToConsole(const char* path)
{
	logErrorNoData("dumpToConsole is unimplemented");
}

void dumpRegisters()
{
	printf("af:\t %X", concat(a, f));
    printf("bc:\t %X", concat(b, c));
    printf("de:\t %X", concat(d, e));
    printf("hl:\t %X", concat(h, l));
    printf("sp:\t %X", sp);
    printf("pc:\t %X", pc);
}

// Initializes the registers to the state they would be in after the bootrom finishes.
void initRegisters() {
	pc = 0x100;
	writePair(&a, &f, 0x01b0);
	writePair(&b, &c, 0x0013);
	writePair(&d, &e, 0x00d8);
	writePair(&h, &l, 0x014d);
	sp = 0xfffe;
	tima = 0;
	tma = 0;
	tmc = 0;
	writeToAddress(0xff40, 0x91);//lcd control
	writeToAddress(0xff47, 0xfc);
	writeToAddress(0xff48, 0xff);
	writeToAddress(0xff49, 0xff);

}

void inc8(uint8_t* x) {

	setHalf(((*x & 0x0f) + (1 & 0x0f)) > 0xf);
    (*x)++;
	setSubtract(false);
	setZero(*x == 0);
	pc++;
	cycles = 4;
}
void dec8(uint8_t* x) {
	setSubtract(true);
	setHalf((int)(*x & 0xf) - (int)(1 & 0xf) < 0);
    (*x)--;
	setZero(*x == 0x00);
	pc++;
	cycles = 4;
}
void add8(uint8_t* destination, uint8_t source) {
	setSubtract(false);
	setCarry((uint16_t)destination + (uint16_t)source > 0xff);
	setHalf(((*destination & 0x0f) + (source & 0x0f)) > 0xf);
	*destination += source;
	setZero(*destination == 0);
	cycles = 4;
	pc++;
}
void sub8(uint8_t* destination, uint8_t source) {
	setSubtract(true);
	setCarry((int)*destination - (int)source < 0);
	setHalf(((int)(*destination & 0x0f) - (int)(source & 0x0f)) < 0x0);
	*destination -= source;
	setZero(*destination == 0);
	cycles = 4;
	pc++;
}
void add16(uint16_t* destination, uint16_t source) {

	setHalf(((*destination & 0xFFF) + (source & 0xFFF)) & 0x1000); //taken from SameBoy
	setCarry(((unsigned long)*destination + (unsigned long)source) & 0x10000);//taken from SameBoy

	*destination += source;
	setSubtract(false);
	cycles = 8;
	pc++;
}
void or8(uint8_t* destination, uint8_t source) {
	setCarry(false);
	setHalf(false);
	setSubtract(false);
	*destination = *destination | source;
	setZero(*destination == 0x00);
	cycles = 4;
	pc++;
}

void switchRamBank(uint8_t number)
{
	if (number != 0)
	{
		cartRam = (uint8_t*)(currentCartridge->ramBanks + (number * ramBankSize));
	}
	else
	{
		cartRam = (uint8_t*)(currentCartridge->ramBanks);
	}
}

void switchBank(uint8_t number) //zero indexed
{
	if (number != 0)
	{
		romSwitchable = (uint8_t*)(currentCartridge->banks + (number * bankSize));
	}
	else
	{
		romSwitchable = (uint8_t*)(currentCartridge->banks + bankSize);
	}

}

void dumpWorkRamToFile(const char* path) {
	logErrorNoData("Dumping workRAM to file is currently unimplemented.");
}

void checkInterrupts() {
	if (halted)
	{
		if (!enableInterrupts && (interruptFlag != 0x00))//if halted and caught interrupt but IME=0
		{
			logWarningNoData("HALT bug.");
			halted = false;
		}
	}
	if (!enableInterrupts)
	{
		return;
	}

	if ((interruptFlag & 0x01) == 0x01 && (interruptRegister & 0x01) == 0x01)//catch v-blank interrupt
	{
		if (LOG_VERBOSE){
            logInfo("Caught V-Blank interrupt.");
		}
		enableInterrupts = false;
		interruptFlag = interruptFlag & 0xfe;//clear vblank bit
		//push pc onto stack
		uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
		uint8_t low = pc & 0x00ff;
		writeToAddress(sp - 1, high);
		writeToAddress(sp - 2, low);
		sp -= 2;
		if (halted)
		{
			halted = false;
			//cout << "WARNING: Ignoring HALT bug...\n";
		}
		pc = 0x40;
	}
	else if ((interruptFlag & 0x02) == 0x02 && (interruptRegister & 0x02) == 0x02)//catch lcd stat interrupt
	{
		if (LOG_VERBOSE)
			logInfo("Caught LCD STAT interrupt.");
		enableInterrupts = false;
		interruptFlag = interruptFlag & 0xfd;
		//push pc onto stack
		uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
		uint8_t low = pc & 0x00ff;
		writeToAddress(sp - 1, high);
		writeToAddress(sp - 2, low);
		sp -= 2;
		if (halted)
		{
			halted = false;
			//cout << "WARNING: Ignoring HALT bug...\n";
		}
		pc = 0x48;
	}
	else if ((interruptFlag & 0x04) == 0x04 && (interruptRegister & 0x04) == 0x04)//catch Timer interrupt
	{
		if (LOG_VERBOSE)
			logInfo("Caught Timer interrupt.");
		//cout<<"Interrupt flag: 0x"<<hex<<(uint16_t)interruptFlag<<dec<<endl;
		enableInterrupts = false;
		interruptFlag = interruptFlag & 0xfb;
		//push pc onto stack
		uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
		uint8_t low = pc & 0x00ff;
		writeToAddress(sp - 1, high);
		writeToAddress(sp - 2, low);
		sp -= 2;
		if (halted)
		{
			halted = false;
			//logWarningNoData("Ignoring HALT bug.");
		}
		pc = 0x50;
	}
	else if ((interruptFlag & 0x08) == 0x08 && (interruptRegister & 0x08) == 0x08)//catch serial interrupt
	{
		//logInfo("Caught serial interrupt.");
		enableInterrupts = false;
		interruptFlag = interruptFlag & 0xf7;
		//push pc onto stack
		uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
		uint8_t low = pc & 0x00ff;
		writeToAddress(sp - 1, high);
		writeToAddress(sp - 2, low);
		sp -= 2;
		if (halted)
		{
			halted = false;
			//logWarningNoData("Ignoring HALT bug.");
		}
		pc = 0x58;
	}
	else if ((interruptFlag & 0x10) == 0x10 && (interruptRegister & 0x10) == 0x10)//catch joypad interrupt
	{
		//logInfo("Caught joypad interrupt.");
		enableInterrupts = false;
		interruptFlag = interruptFlag & 0xef;
		//push pc onto stack
		uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
		uint8_t low = pc & 0x00ff;
		writeToAddress(sp - 1, high);
		writeToAddress(sp - 2, low);
		sp -= 2;
		if (halted)
		{
			halted = false;
			//logWarningNoData("Ignoring HALT bug.");
		}
		pc = 0x60;
	}

}
void processTimer(uint8_t opCycle)
{
	doDividerRegister(opCycle);
	if (TestBit(tmc, 2))
	{
		m_TimerCounter -= opCycle;
		if (m_TimerCounter <= 0)
		{
			SetClockFreq();
			if (tima == 0xff)
			{
				writeToAddress(0xff05, readFromAddress(0xff06));

				if (LOG_VERBOSE)
					logInfo("Requesting timer interrupt.");
				interruptFlag |= 0x04;
			}
			else
			{
				tima = tima + 1;
			}
		}
	}
}
void doDividerRegister(uint8_t opCycle)
{
	m_DividerCounter += opCycle;
	if (m_DividerCounter >= 255)
	{
		m_DividerCounter = 0;
		m_DividerRegister++;
	}
}
uint8_t GetClockFreq()
{
	return tmc & 0x03;
}
void SetClockFreq()
{
	uint8_t freq = GetClockFreq();
	switch (freq)
	{
	case 0: m_TimerCounter = 1024; break;
	case 1: m_TimerCounter = 16; break;
	case 2: m_TimerCounter = 64; break;
	case 3: m_TimerCounter = 256; break;
	default: logErrorNoData("Invalid clock freq");
	}
}

void doDMATransfer(uint8_t data) {

	uint16_t address = data << 8;
	for (uint8_t i = 0; i < 0xa0; i++)
	{
		writeToAddress(0xfe00 + i, readFromAddress(address + i));
	}
}

