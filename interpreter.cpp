#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-exception-baseclass"
#pragma ide diagnostic ignored "readability-static-accessed-through-instance"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by andrew on 11/17/19.
//
#include <iostream>
#include <fstream>
//#include <unistd.h>
#include "cartridge.h"
#include "interpreter.h"
#include "lcdController.h"
#include "keyboardInput.h"
#include "main.h"

using namespace std;

int errorAddress = -1;
std::string output = "Program output: \n";
bool enableInterrupts = true;
bool lcdEnable = true;
bool windowDisplayEnable = true;
bool spriteDisplayEnable = true;
bool bgDisplayEnable = true;
bool spriteSize, bgTilemapDisplaySelect, bgAndWindowTileDataSelect, windowTilemapDisplaySelect;
bool ramEnable;
unsigned int cycles = 0;
double cycleTime = 0.23841880647;//microseconds
unsigned int clockSpeed = 4194304;//4.2 mhz
int m_TimerCounter;
uint8_t a,b,c,d,e,f,h,l;
uint16_t sp;
uint16_t pc;

uint8_t vram[8192];
uint8_t* cartRam;
uint8_t workRam[8192];
uint8_t* rom0;//first 256 are internal bootROM
uint8_t* romSwitchable;
uint8_t backgroundPalette, scrollX, scrollY, windowX, windowY, lcdControl, coincidence, lcdStatus, objectPalette0Data, objectPalette1Data;;
uint8_t highRam[127];
uint8_t spriteAttributeTable[160];

uint8_t joypadRegister;
uint8_t interruptRegister = 0;
uint8_t interruptFlag = 0;

uint8_t m_DividerRegister = 0;
unsigned int m_DividerCounter;
uint8_t serialControl;
uint8_t tima,tma,tmc;
cartridge* currentCartridge;
char tempOutput;
bool halted = false;


void writeToAddress(uint16_t address, uint8_t data) {
    // std::cout<<"Writing address " << std::to_string(address) <<std::endl;
    if (address >= 0x8000 && address <=0x9fff)
    {
        vram[address-0x8000] = data;
    }
    else if (address == 0xff7f)
    {
        cout<<"Wrote to 0xff7f, probably a ROM bug."<<endl;
    }
    else if (address >= 0xc000 && address <= 0xdfff)
    {
        //cout<<"INFO: Wrote 0x"<<hex<<(uint16_t)(data)<<dec<<" to work ram address 0x"<<hex<<(address)<<dec<<endl;
        workRam[address-0xc000] = data;
    }
    else if (address >= 0xe000 && address <=0xfdff)//mirror
    {
        workRam[address-0xe000] = data;
    }
    else if (address <= 0x7FFF)//write to ROM
    {
        handleRomWrite(address, data);
    }
    else if (address >= 0xff10 && address <= 0xff3f)//sound
    {
        handleSoundWrite(address,data);
    }
    else if (address >= 0xa000 && address <= 0xbfff)
    {
        if (ramEnable)
        {
            if (currentCartridge->ramBankIdentifier == 0x00)//no cartRAM
            {
                cout<<"WARNING: Attempting to write to cartRAM on cartridge without RAM, ignoring...\n";
            } else{
                cartRam[address - 0xa000] = data;
            }
        } else
        {
            cout<<"WARNING: Attempting to write to cartRAM while RAM is disabled, ignoring...\n";
        }
        //cout<<"WARNING: Cart RAM banking is not tested. Writing to address 0x" <<hex<<address<<dec<<".\n";

    }
    else if (address >= 0xff00 && address <= 0xff7f)
    {
        handleIOWrite(address, data);
    }
    else if (address >= 0xff80 && address <= 0xfffe)
    {
        highRam[address-0xff80] = data;
    }
    else if (address == 0xffff)//interrupts
    {
        interruptRegister = data;
    }
    else if (address >= 0xfea0 && address <= 0xfeff)
    {
        cout<<"Tried to write to an unused address, probably a ROM bug."<<endl;
    }
    else if (address >= 0xfe00 && address <= 0xfe9f)
    {
        //cout<<"Writing to sprite attribute table address "<<to_string(0xfe00-address)<<endl;
        spriteAttributeTable[address-0xfe00] = data;
    }
    else{
        errorAddress = address;
        throw "WRITE ADDRESS NOT IMPLEMENTED";
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
        return romSwitchable[address-0x4000];
    }
    else if (address >= 0x8000 && address <=0x9fff)
    {
        return vram[address-0x8000];
    }
    else if (address >= 0xc000 && address <= 0xdfff)
    {
        return workRam[address-0xc000];
    }
    else if (address >= 0xe000 && address <=0xfdff)//mirror
    {
        return workRam[address-0xe000];
    }
    else if (address >= 0xa000 && address <= 0xbfff)
    {
        if (ramEnable)
        {
            if (currentCartridge->ramBankIdentifier == 0x00)//no cartRAM
            {
                cout<<"WARNING: Attempting to read from cartRAM on cartridge without RAM, returning 0xff...\n";
                return 0xff;
            } else{
                return cartRam[address - 0xa000];
            }
        } else
        {
            cout<<"WARNING: Attempting to read from cartRAM while RAM is disabled, returning 0xff...\n";
            return 0xff;
        }
    }
    else if (address >= 0xff80 && address <= 0xfffe)
    {
        return highRam[address-0xff80];
    }
    else if (address == 0xffff)//interrupts
    {
        return interruptRegister;
    }
    else if (address >= 0xff10 && address <= 0xff3f)//sound
    {
        return handleSoundRead(address);
    }
    else if ((address >= 0xff00 && address < 0xff10) || (address>0xff3f && address <= 0xff7f))
    {
        return handleIORead(address);
    }
    else if (address >= 0xfea0 && address <= 0xfeff)
    {
        cout<<"Tried to write to an unused address, probably a ROM bug."<<endl;
        return 0xff;
    }
    else if (address >= 0xfe00 && address <= 0xfe9f)
    {
        return spriteAttributeTable[0xfe00-address];
    }
    else{
        errorAddress = address;
        throw "READ ADDRESS NOT IMPLEMENTED";
    }
}

/*
 * Memory map:
 * 0x0000-0x3FFF: ROM Bank 0 (16k)
 * 0x4000-0x7FFF: Switchable ROM bank (16k)
 * 0x8000-0x9FFF: Video RAM (8k)
 * 0xA000-0xBFFF: Switchable RAM AKA .sav (8k)
 * 0xC000-0xCFFF: Work RAM bank 0 (4k)
 * 0xD000-0xDFFF: Work RAM bank 1 (4k)
 * 0xE000-0xFDFF: Echo of work RAM (8k)
 * 0xFE00-0xFE9F: Sprite Attribute table (256)
 * 0xFEA0-0xFEFF: Not mapped
 * 0xFF00-0xFF7F: Device mappings (128)
 * 0xFF80-0xFFFE: High RAM (127)
 * 0xFFFF-0xFFFF: Interrupt enable
 */

void handleRomWrite(uint16_t address, uint8_t data)
{
    if (currentCartridge->mbcType == 0x00 )//ROM only, 2 banks
    {
        if (data == 0x01)
        {
            cout<<"WARNING: Attempting to select ROM bank 1 in cart without MBC.\n";
        } else{
            errorAddress = address;
            cout<<"WARNING: Attempting to select invalid ROM bank in cart without MBC.\n";
            throw "Tried to select an invalid ROM bank on cart without MBC";
        }
    }
    else if (currentCartridge->mbcType == 0x01)//mbc1
    {
        if (address >= 0x2000 && address <= 0x3fff)//bank select
        {
            switchBank(data);
            //cout<<"DEBUG: Selected ROM bank "<<to_string(data) <<hex<<"\t0x"<<(uint16_t)(data)<<dec<<endl;
            //usleep(500000);
        }
        else if (address <= 0x1fff)//ram enable
        {
            cout<<"WARNING: RAM enable and banking is not tested.\n";
            if ((data & 0x0f) == 0x0a)
            {
                cout<<"INFO: enabling cartRAM.\n";
                ramEnable = true;
            }
            else
            {
                cout<<"INFO: disabling cartRAM.\n";
                ramEnable = false;
            }
        }
        else
        {
            cout<<"Error writing to address 0x"<<hex<<address<<dec<<" with data 0x"<<hex<<(uint16_t)data<<dec<<"\n";
            errorAddress = address;
            throw "Wrote to invalid ROM address on mbc1 only";
        }
    }
    else if (currentCartridge->mbcType == 0x03)//mbc1 + RAM + Battery
    {
        if (address >= 0x2000 && address <= 0x3fff)//bank select
        {
            switchBank(data);
            //cout<<"DEBUG: Selected ROM bank "<<to_string(data) <<hex<<"\t0x"<<(uint16_t)(data)<<dec<<endl;
            //usleep(500000);
        }
        else if (address <= 0x1fff)//ram enable
        {
            cout<<"WARNING: RAM enable and banking is not tested.\n";
            if ((data & 0x0f) == 0x0a)
            {
                cout<<"INFO: enabling cartRAM.\n";
                ramEnable = true;
            }
            else
            {
                cout<<"INFO: disabling cartRAM.\n";
                ramEnable = false;
            }
        }
        else
        {
            cout<<"Error writing to address 0x"<<hex<<address<<dec<<" with data 0x"<<hex<<(uint16_t)data<<dec<<"\n";
            errorAddress = address;
            throw "Wrote to unimplemented ROM address on mbc1+ram+battery";
        }
    }
    else
    {
        cout<<"Error writing to address 0x"<<hex<<address<<dec<<" with data 0x"<<hex<<(uint16_t)data<<dec<<"\n";
        errorAddress = address;
        throw "Attempting to write to ROM on unimplemented MBC type.";
    }
}

uint16_t concat(uint8_t high, uint8_t low) {
    return high*256 + low;
}

void writePair(uint8_t &x, uint8_t &y, uint16_t data) {
    //cout<<"data: " <<to_string(data) <<endl<<"x before: " <<to_string(x)<<endl;
    x = ((data & 0xff00) >> 8);
    //cout<<" and after: " <<to_string(x)<<endl;
    //cout<<"y before: " <<to_string(y)<<endl;
    y = data & 0x00ff;
    //cout<<"and after " <<to_string(y)<<endl;
    //cout<<"concat after: " <<to_string(concat(x,y))<<endl;
}

void handleSoundWrite(uint16_t address, uint8_t data)
{
    cout<<"write to sound registers ignored (address: "<<to_string(address) <<", data: " <<to_string(data)<<endl;
}
uint8_t handleSoundRead(uint16_t address)
{
    cout << "read from sound registers ignored, returning 0x00 (address: "<<to_string(address)<<endl;
    return 0x00;
}

void handleIOWrite(uint16_t address, uint8_t data) {
    if (address == 0xff4d)
    {
        cout<<"\033[1;33mWARNING:\033[0m Speed control register write unimplemented."<<endl;
    }
    else if (address >= 0xff03 && address <= 0xff0e)
    {
        cout<<"WARNING: Tried writing to 0xff03-0xff0e, ignoring...\n";
    }
    else if (address == 0xff00)
    {
        joypadRegister = data;
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
	    windowX = data;
    }
    else if (address == 0xff40)
    {
        lcdControl = data;
        bgDisplayEnable = (lcdControl & 0x01) == 0x01;//test for bg display enable bit
        spriteDisplayEnable = (lcdControl & 0x02) == 0x02;//...
        spriteSize = (lcdControl & 0x04) == 0x04;//...
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
        if (TestBit(tmc,2))
        {
            cout<<"INFO: timer is enabled.\n";
        }
        else
        {
            cout<<"INFO: timer is disabled.\n";
        }
    }
    else if (address == 0xff04)
    {
        m_DividerRegister = 0;
    }
    else if (address == 0xff0f)
    {
       // cout<<"INFO: Writing to interrupt flag at pc 0x" <<hex<<(pc)<<dec<<" and data 0x"<<hex<<(uint16_t)(data)<<dec<<endl;
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
        cout<<"\033[1;33mWARNING:\033[0m Serial control byte not implemented."<<endl;
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
    else
    {
        errorAddress = address;
        throw "IO WRITE NOT IMPLEMENTED";
    }
}
uint8_t handleIORead(uint16_t address) {
    if (address == 0xff00)
    {
        return getJoypadState();
    }
    else if (address == 0xff01)
    {
        return tempOutput;
    }
    else if (address >= 0xff03 && address <= 0xff0e)
    {
        cout<<"WARNING: reading from 0xff03-0xff0e, returning 0xff...\n";
        return 0xff;
    }
    else if (address == 0xff44)//lcdc y coordinate (0-153, 144-153 is vblank)
    {
        //cout<<"\033[1;33mWARNING:\033[0m LCD ypos byte read unimplemented "<<endl;
        return line;
    }
    else if (address == 0xff40)
    {
        return lcdControl;
    }
    else if (address == 0xff4d)//cgb key1 (prepare speed switch)
    {
        cout<<"\033[1;33mWARNING:\033[0m Speed control register unimplemented, returning 0xff "<<endl;
        return 0xff;
    }
    else if (address == 0xff45)
    {
        return coincidence;
    }
    else if (address == 0xff47)
    {
        return backgroundPalette;
    }
    else if (address == 0xff41)
    {
        return lcdStatus;
    }
    else if (address == 0xff0f)
    {
        return interruptFlag | 0xe0;//set top 3 bits to always be true
    }
    /*else if (address == 0xff47)
    {
        return bgPaletteData;
    }*/
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
    else
    {
        errorAddress = address;
        throw "IO READ NOT IMPLEMENTED";
    }
}

void loadTestRom(string path)//calculates number of banks based on cartridge MBC number
{
	std::cout << "Loading test ROM: " << path << "...\n";
    std::ifstream infile(path);
    if (!infile.good())
    {
        cout<<"ROM file not found!"<<endl;
        throw "ROM not found, exiting...";
    }
    uint8_t mbc, banks, ramBanks;

    infile.seekg(0,std::ios::end);
    size_t length = infile.tellg();
    infile.seekg(0x0147,std::ios::beg);
    infile.read((char*)(&mbc),1);
    infile.read((char*)(&banks),1);
    infile.read((char*)(&ramBanks),1);

    cout<<"ROM MBC: "<<to_string(mbc)<<endl<<"ROM Bank identifier: "<<to_string(banks)<<endl;
    cout<<"RAM banks identifier: "<<to_string(ramBanks)<<endl;
    currentCartridge = new cartridge(mbc, banks, ramBanks);
    infile.seekg(0,std::ios::beg);
    cout<<"File size is "<<to_string(length)<<" bytes."<<endl;
    infile.read((char*)(currentCartridge->banks),length);
    rom0 = currentCartridge->banks;//pointer to beginning of banks
    cartRam = currentCartridge->ramBanks;
    romSwitchable = (uint8_t*)(currentCartridge->banks + currentCartridge->bankSize);//romSwitchable points to 2nd bank at first
    //cout<<"Romswitchable and rom0: "<<hex<<"0x"<<(unsigned long)(romSwitchable)<<" 0x"<<(unsigned long)(rom0)<<dec<<endl;
    //cout<<"romSwitchable is now 0x" <<hex<<((unsigned long)(romSwitchable-rom0))<<dec<<" bytes more than rom0"<<endl;
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
    } else{
        f &= 0xef;
    }
}
void setHalf(bool value)
{
    if (value)
    {
        f |= 0x20;
    } else{
        f &= 0xdf;
    }
}
void setSubtract(bool value)
{
    if (value)
    {
        f |= 0x40;
    } else{
        f &= 0xbf;
    }
}
void setZero(bool value)
{
    if (value)
    {
        f |= 0x80;
    } else{
        f &= 0x7f;
    }
}

void dumpToConsole(std::string path)
{
    ofstream outfile(path);
    outfile.write((char*)rom0,0x4000);
    outfile.close();
}

void dumpRegisters()
{
    cout<<"pc:\t" <<to_string(pc)<<"\t"<<"0x"<<hex<<pc<<dec<<endl;
    cout<<"sp:\t" <<to_string(sp)<<"\t"<<"0x"<<hex<<sp<<dec<<endl;
    cout<<"a:\t" <<to_string(a)<<"\t"<<"0x"<<hex<<(int)a<<dec<<endl;
    cout<<"b:\t" <<to_string(b)<<"\t"<<"0x"<<hex<<(int)b<<dec<<endl;
    cout<<"c:\t" <<to_string(c)<<"\t"<<"0x"<<hex<<(int)c<<dec<<endl;
    cout<<"d:\t" <<to_string(d)<<"\t"<<"0x"<<hex<<(int)d<<dec<<endl;
    cout<<"e:\t" <<to_string(e)<<"\t"<<"0x"<<hex<<(int)e<<dec<<endl;
    cout<<"de:\t" <<to_string(concat(d,e))<<"\t"<<"0x"<<hex<<concat(d,e)<<dec<<endl;
    cout<<"f:\t" <<to_string(f)<<"\t"<<"0x"<<hex<<(int)f<<dec<<endl;
    cout<<"h:\t" <<to_string(h)<<"\t"<<"0x"<<hex<<(int)h<<dec<<endl;
    cout<<"l:\t" <<to_string(l)<<"\t"<<"0x"<<hex<<(int)l<<dec<<endl;
    cout<<"hl:\t" <<to_string(concat(h,l))<<"\t"<<"0x"<<hex<<concat(h,l)<<dec<<endl;

}
void initRegisters() {
    writePair(a,f,0x01b0);
    writePair(b,c,0x0013);
    writePair(d,e,0x00d8);
    writePair(h,l,0x014d);
    sp = 0xfffe;
    tima = 0;
    tma = 0;
    tmc = 0;
    writeToAddress(0xff40, 0x91);//lcd control
    writeToAddress(0xff47,0xfc);
    writeToAddress(0xff48,0xff);
    writeToAddress(0xff49,0xff);


}

void inc8(uint8_t &x) {

    setHalf(((x & 0x0f) + (1 & 0x0f)) > 0xf);
    x++;
    setSubtract(false);
    setZero(x==0);
    pc++;
    cycles = 4;
}
void dec8(uint8_t &x) {
    setSubtract(true);
    setHalf((int)(x & 0xf) - (int)(1&0xf) < 0);
    x--;
    setZero(x==0x00);
    pc++;
    cycles = 4;
}
void add8(uint8_t& destination, uint8_t source) {
    setSubtract(false);
    setCarry((uint16_t )destination + (uint16_t)source > 0xff);
    setHalf(((destination & 0x0f) + (source & 0x0f)) > 0xf);
    destination += source;
    setZero(destination == 0);
    cycles = 4;
    pc++;
}
void sub8(uint8_t & destination, uint8_t source) {
    setSubtract(true);
    setCarry((int)destination - (int)source < 0);
    setHalf(((int)(destination & 0x0f) - (int)(source & 0x0f)) < 0x0);
    destination -= source;
    setZero(destination == 0);
    cycles = 4;
    pc++;
}
void add16(uint16_t &destination, uint16_t source) {
    //cout<<"\033[1;33mWARNING:\033[0m Half-carry with 16-bit adds is untested."<<endl;
    setCarry((unsigned int)destination + (unsigned int)source > 0xffff   || (source&0xffff + destination&0xffff)>0xffff);
    setHalf(((destination & 0x0f) + (source & 0x0f)) > 0xf || ((destination & 0x0f00) + (source & 0x0f00)) > 0x0f00);//check carry from bit 4 to 5 and from 11 to 12
    //ONLY TESTS FIRST NIBBLE!
    destination += source;
    setSubtract(false);
    cycles = 8;
    pc++;
}
void or8(uint8_t &destination, uint8_t source) {
    setCarry(false);
    setHalf(false);
    setSubtract(false);
    destination = destination | source;
    setZero(destination==0x00);
    cycles = 4;
    pc++;
}

void switchBank(uint8_t number) //zero indexed
{
    if (number != 0)
    {
        romSwitchable = (uint8_t*)(currentCartridge->banks + (number * currentCartridge->bankSize));
    }
    else
    {
        romSwitchable = (uint8_t*)(currentCartridge->banks + (currentCartridge->bankSize));
    }

}

void dumpWorkRamToFile(std::string path) {
    ofstream outfile(path);
    outfile.write((char*)workRam,sizeof(workRam));
    outfile.close();
}

void checkInterrupts() {
    if (halted)
    {
        if (!enableInterrupts && (interruptFlag != 0x00))//if halted and caught interrupt but IME=0
        {
            cout<<"WARNING: HALT bug.\n";
            halted = false;
        }
    }
    if (!enableInterrupts)
    {
        return;
    }

    if ((interruptFlag & 0x01) == 0x01 && (interruptRegister & 0x01) == 0x01)//catch v-blank interrupt
    {
        cout<<"\033[1;32mINFO: \033[0mCaught V-blank interrupt."<<endl;
        enableInterrupts = false;
        interruptFlag = interruptFlag & 0xfe;//clear vblank bit
        //push pc onto stack
        uint8_t high = (((uint16_t )pc & 0xff00)>>8);
        uint8_t low = pc & 0x00ff;
        writeToAddress(sp-1,high);
        writeToAddress(sp-2,low);
        sp-=2;
        if (halted)
        {
            halted = false;
            cout<<"WARNING: Ignoring HALT bug...\n";
        }
        pc=0x40;
    }
    else if ((interruptFlag & 0x02) == 0x02&& (interruptRegister & 0x02) == 0x02)//catch lcd stat interrupt
    {
        cout<<"\033[1;32mINFO: \033[0mCaught LCD STAT interrupt."<<endl;
        enableInterrupts = false;
        interruptFlag = interruptFlag & 0xfd;
        //push pc onto stack
        uint8_t high = (((uint16_t )pc & 0xff00)>>8);
        uint8_t low = pc & 0x00ff;
        writeToAddress(sp-1,high);
        writeToAddress(sp-2,low);
        sp-=2;
        if (halted)
        {
            halted = false;
            cout<<"WARNING: Ignoring HALT bug...\n";
        }
        pc=0x48;
    }
    else if ((interruptFlag & 0x04) == 0x04 && (interruptRegister & 0x04) == 0x04)//catch Timer interrupt
    {
        cout<<"\033[1;32mINFO: \033[0mCaught timer interrupt."<<endl;
        cout<<"Interrupt flag: 0x"<<hex<<(uint16_t)interruptFlag<<dec<<endl;
        enableInterrupts = false;
        interruptFlag = interruptFlag & 0xfb;
        //push pc onto stack
        uint8_t high = (((uint16_t )pc & 0xff00)>>8);
        uint8_t low = pc & 0x00ff;
        writeToAddress(sp-1,high);
        writeToAddress(sp-2,low);
        sp-=2;
        if (halted)
        {
            halted = false;
            cout<<"WARNING: Ignoring HALT bug...\n";
        }
        pc=0x50;
    }
    else if ((interruptFlag & 0x08) == 0x08 && (interruptRegister & 0x08) == 0x08)//catch serial interrupt
    {
        cout<<"\033[1;32mINFO: \033[0mCaught serial interrupt."<<endl;
        enableInterrupts = false;
        interruptFlag = interruptFlag & 0xf7;
        //push pc onto stack
        uint8_t high = (((uint16_t )pc & 0xff00)>>8);
        uint8_t low = pc & 0x00ff;
        writeToAddress(sp-1,high);
        writeToAddress(sp-2,low);
        sp-=2;
        if (halted)
        {
            halted = false;
            cout<<"WARNING: Ignoring HALT bug...\n";
        }
        pc=0x58;
    }
    else if ((interruptFlag & 0x10) == 0x10 && (interruptRegister & 0x10) == 0x10)//catch joypad interrupt
    {
        cout<<"\033[1;32mINFO: \033[0mCaught joypad interrupt."<<endl;
        enableInterrupts = false;
        interruptFlag = interruptFlag & 0xef;
        //push pc onto stack
        uint8_t high = (((uint16_t )pc & 0xff00)>>8);
        uint8_t low = pc & 0x00ff;
        writeToAddress(sp-1,high);
        writeToAddress(sp-2,low);
        sp-=2;
        if (halted)
        {
            halted = false;
            cout<<"WARNING: Ignoring HALT bug...\n";
        }
        pc=0x60;
    }

}
void processTimer(uint8_t opCycle)
{
    doDividerRegister(opCycle);
    if (TestBit(tmc,2))
    {
        m_TimerCounter -= opCycle;
        if (m_TimerCounter <= 0)
        {
            SetClockFreq();
            if(tima == 0xff)
            {
                writeToAddress(0xff05, readFromAddress(0xff06));
                cout<<"INFO: Requesting timer interrupt.\n";
                interruptFlag |= 0x04;
            }
            else
            {
                tima = tima+1;
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
    return tmc&0x03;
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
        default: throw "Invalid clock freq";
    }
}

void doDMATransfer(uint8_t data) {

    //dumpRegisters();
    uint16_t address = data << 8;

    cout<<"\033[1;32mINFO: \033[0mExecuting OAM DMA transfer, reading from addresses 0x"<<hex<<address<<"-0x"<<address+0xa0<<dec<<endl;
    for (uint8_t i = 0; i<0xa0; i++)
    {
        writeToAddress(0xfe00+i, readFromAddress(address+i));
    }
}

#pragma clang diagnostic pop