/*
 *     GBEmu - A simple DMG emulator
 *     Copyright (C) 2022 Andrew Watson
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma ide diagnostic ignored "hicpp-exception-baseclass"
#pragma ide diagnostic ignored "readability-static-accessed-through-instance"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

#include <iostream>
#include <fstream>

#include "cartridge.h"
#include "interpreter.h"
#include "lcdController.h"
#include "keyboardInput.h"
#include "instructionDecoder.h"
#include "logger.h"
#include "exceptions.h"





//Define as true to show instructions being run
#define DEBUG_MODE false


using namespace std;


uint8_t targetRegister;  
uint8_t instruction;   
bool memoryReference;
uint8_t temp;
uint8_t* reg;
void execute(uint16_t address)
{
	


	if (halted)
	{
		cycles = 4;
		processTimer(cycles);
		checkInterrupts();
		return;
	}
	uint8_t opcode;

	if (runningTest)
	{
		opcode = testInstructions[address];
	}
	else
	{
		opcode = readFromAddress(address);
	}

	if (DEBUG_MODE)
	{
		cout << "DEBUG: Running instruction at address:  0x" << hex << address << "   opcode: 0x" << (uint16_t)opcode << dec << "\n";
	}

	targetRegister = opcode & 0x07;     //bottom 3 bits are which register (b,c,d,e,h,l,(hl),a
	instruction = (opcode & 0xf8) >> 3;   //top 5 bits are instruction (rlc,rrc,rl,rr,sla,sra,swap,srl,bit 0-7,res 0-7,set 0-7)
	memoryReference = false;





	if (opcode >= 0x40 && opcode <= 0xbf && opcode != 0x76)//loads, add, adc, sub, sbc, and, xor, or, cp
	{
		if (targetRegister == 0x06)
		{
			memoryReference = true;
		}
		if (!memoryReference)
		{
			switch (targetRegister)
			{
			case 0:
				reg = &b;
				break;
			case 1:
				reg = &c;
				break;
			case 2:
				reg = &d;
				break;
			case 3:
				reg = &e;
				break;
			case 4:
				reg = &h;
				break;
			case 5:
				reg = &l;
				break;
			case 7:
				reg = &a;
				break;
			default:
				throw exceptions::invalidInstruction("Error in middle chunk of instructions, invalid target register");
			}
		}
		else //read from address in hl
		{
			temp = readFromAddress(concat(h, l));
			reg = &temp;
		}
	}
	if (opcode >= 0x40 && opcode <= 0x7f && opcode != 0x76)//all loads
	{
		bool memoryTarget = false;
		uint8_t* dest;
		switch (opcode & 0x78)//bits 3-6
		{
		case 0x40://ld b, x
			dest = &b;
			break;
		case 0x48:
			dest = &c;
			break;
		case 0x50:
			dest = &d;
			break;
		case 0x58:
			dest = &e;
			break;
		case 0x60:
			dest = &h;
			break;
		case 0x68:
			dest = &l;
			break;
		case 0x70:
			dest = &temp;
			memoryTarget = true;
			break;
		case 0x78:
			dest = &a;
			break;
		default:
			dest = &a;
			logger::logError("Invalid destination for load!", address, opcode);
			break;
		}

		*dest = *reg;

		if (memoryTarget)
		{
			if (memoryReference)
			{
				logger::logError("Error in instruction decoder", address, opcode);
				throw exceptions::invalidInstruction("Error in instruction decoder");
			}
			writeToAddress(concat(h, l), *dest);
		}

		pc++;
		cycles = 4;
		if (memoryReference || memoryTarget)
			cycles = 8;
	}
	else if (opcode >= 0x80 && opcode <= 0x87)//add a, x
	{
		add8(a, *reg);
		if (memoryReference)
		{
			cycles = 8;
		}
	}
	else if (opcode >= 0x88 && opcode <= 0x8f)//adc a, x
	{
		if (carryStatus())
		{
			setHalf((a & 0xF) + (*reg & 0xF) + 1 > 0x0F);
			setCarry(((unsigned long)a) + ((unsigned long)(*reg)) + 1 > 0xFF);
			a += *reg;
			a++;
		}
		else
		{
			setHalf((a & 0xF) + (*reg & 0xF) > 0x0F);
			setCarry(((unsigned long)a) + ((unsigned long)(*reg)) > 0xFF);
			a += *reg;
		}
		setZero(a == 0);
		setSubtract(false);
		if (memoryReference)
		{
			cycles = 8;
		}
		else
		{
			cycles = 4;
		}
		pc++;
		
	}
	else if (opcode >= 0x90 && opcode <= 0x97)//sub a, x
	{
		sub8(a, *reg);
		if (memoryReference)
			cycles = 8;
	}
	else if (opcode >= 0x98 && opcode <= 0x9f)//sbc a, x
	{
		if (carryStatus())
		{
			setHalf((a & 0xF) < (*reg & 0xF) + 1);
			setCarry(((unsigned long)a) - ((unsigned long)(*reg)) - 1 > 0xFF);
			a -= *reg;
			a--;
		}
		else
		{
			setHalf((a & 0xF) < (*reg & 0xF));
			setCarry(((unsigned long)a) - ((unsigned long)(*reg)) > 0xFF);
			a -= *reg;
		}
		setZero(a == 0);
		setSubtract(true);
		if (memoryReference)
		{
			cycles = 8;
		}
		else
		{
			cycles = 4;
		}
		pc++;
	}
	else if (opcode >= 0xa0 && opcode <= 0xa7)//and a, x
	{
		a = a & (*reg);
		setZero(a == 0x00);
		setSubtract(false);
		setHalf(true);
		setCarry(false);
		pc++;
		cycles = 4;
		if (memoryReference)
			cycles = 8;
	}
	else if (opcode >= 0xa8 && opcode <= 0xaf)//xor a, x
	{
		a = a ^ (*reg);
		setZero(a == 0x00);
		setSubtract(false);
		setHalf(false);
		setCarry(false);
		pc++;
		cycles = 4;
		if (memoryReference)
			cycles = 8;
	}
	else if (opcode >= 0xb0 && opcode <= 0xb7)// or a, x
	{
		or8(a, *reg);
		if (memoryReference)
			cycles = 8;
	}
	else if (opcode >= 0xb8 && opcode <= 0xbf)//cp a, x
	{
		uint8_t temp = a;
		sub8(a, *reg);
		a = temp;
		if (memoryReference)
			cycles = 8;
	}
	else if (opcode == 0x31)//ld sp, nn
	{

		sp = concat(readFromAddress(address + 2), readFromAddress(address + 1));//read nn into sp
		pc += 3;
		cycles = 12;
	}
	else if (opcode == 0x00)//nop
	{
		pc++;
		cycles = 4;
	}
	else if (opcode == 0xee)//xor a, n
	{
		a = a ^ readFromAddress(address + 1);
		setCarry(false);
		setSubtract(false);
		setHalf(false);
		setZero(a == 0x00);
		pc += 2;
		cycles = 8;
	}
	else if (opcode == 0xf6)//or a, n
	{
		a = a | readFromAddress(address + 1);
		setCarry(false);
		setSubtract(false);
		setHalf(false);
		setZero(a == 0x00);
		pc += 2;
		cycles = 8;
	}
	else if (opcode == 0xc6)//add a, n
	{
		add8(a, readFromAddress(address + 1));
		cycles = 8;
		pc++; //needed so that final pc is +=2
	}
	else if (opcode == 0x29)//add hl, hl
	{
		uint16_t temp = concat(h, l);
		add16(temp, temp);
		writePair(h, l, temp);
	}
	else if (opcode == 0x09)//add hl, bc
	{
		uint16_t temp = concat(h, l);
		add16(temp, concat(b, c));
		writePair(h, l, temp);
	}
	else if (opcode == 0x19)//add hl, de
	{
		uint16_t temp = concat(h, l);
		add16(temp, concat(d, e));
		writePair(h, l, temp);
	}
	else if (opcode == 0x39)//add hl, sp
	{
		uint16_t temp = concat(h, l);
		add16(temp, sp);
		writePair(h, l, temp);
	}
	else if (opcode == 0xd6)//sub a, n
	{
		sub8(a, readFromAddress(address + 1));
		cycles = 8;
		pc++; //needed so that final pc is +=2
	}
	else if (opcode == 0x21)//ld hl, nn
	{
		l = readFromAddress(address + 1);//read n into l
		h = readFromAddress(address + 2);//read n into h
		pc += 3;
		cycles = 12;
	}
	else if (opcode == 0x32)//ldd (hl),a
	{
		writeToAddress(concat(h, l), a);
		writePair(h, l, concat(h, l) - 1);
		cycles = 8;
		pc++;
	}
	else if (opcode == 0xe2)//ldh (c),a (or ld (ff00+c), a)
	{
		writeToAddress(0xff00 + c, a);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0xf2)//ldh a, (c) (or ld a, (ff00+c))
	{
		a = readFromAddress(0xff00 + c);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x36)//ld (hl), n
	{
		writeToAddress(concat(h, l), readFromAddress(address + 1));
		pc += 2;
		cycles = 12;
	}
	else if (opcode == 0xe0)//ld (0xff00+n), a
	{
		writeToAddress(0xff00 + readFromAddress(address + 1), a);
		pc += 2;
		cycles = 12;
	}
	else if (opcode == 0x11) //ld de, nn
	{
		d = readFromAddress(address + 2);//read n into d
		e = readFromAddress(address + 1);//read n into e
		pc += 3;
		cycles = 12;
	}
	else if (opcode == 0x1a)//ld a, (de)
	{
		a = (readFromAddress(concat(d, e)));
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x0a)//ld a, (bc)
	{
		a = (readFromAddress(concat(b, c)));
		pc++;
		cycles = 8;
	}
	else if (opcode == 0xe8)//add sp, i8
	{
		int8_t next = (int8_t)readFromAddress(address + 1);
		setCarry((((uint16_t)sp & 0xFF) + ((int16_t)next & 0xFF) > 0xFF));
		setHalf(((sp & 0x0f) + (next & 0x0f)) > 0xf );	//check only bit 3 to 4 --->>https://stackoverflow.com/questions/57958631/game-boy-half-carry-flag-and-16-bit-instructions-especially-opcode-0xe8
		setSubtract(false);
		setZero(false);
		sp += next;
		cycles = 16;
		pc += 2;
	}
	else if (opcode == 0xde)//sbc a, u8
	{
		uint8_t next = readFromAddress(address + 1);
		if (carryStatus())
		{
			setHalf((a & 0xf) < (next & 0xf) + 1);
			setCarry(((unsigned long)a) - ((unsigned long)next) - 1 > 0xFF);
			a--;
		}
		else
		{
			setCarry(((unsigned long)a) - ((unsigned long)next) > 0xFF);
			setHalf((a & 0xf) < (next & 0xf));
		}

		a -= next;
		setZero(a == 0);
		setSubtract(true);
		cycles = 8;
		pc+=2;//memory read
	}
	else if (opcode == 0xfa)//ld a, (nn)
	{
		a = (readFromAddress(concat(readFromAddress(address + 2), readFromAddress(address + 1))));
		pc += 3;
		cycles = 16;
	}
	else if (opcode == 0xe6)//and a, n
	{
		a = a & readFromAddress(address + 1);
		setZero(a == 0x00);
		setSubtract(false);
		setHalf(true);
		setCarry(false);
		pc += 2;
		cycles = 8;
	}
	else if (opcode == 0x0e)//ld c,n
	{
		c = readFromAddress(address + 1);
		pc += 2;
		cycles = 8;
	}
	else if (opcode == 0x3e)//ld a,n
	{
		a = readFromAddress(address + 1);
		pc += 2;
		cycles = 8;
	}
	else if (opcode == 0x06)//ld b, n
	{
		b = readFromAddress(address + 1);
		pc += 2;
		cycles = 8;
	}

	else if (opcode == 0x16)//ld d, n
	{
		d = readFromAddress(address + 1);
		pc += 2;
		cycles = 8;
	}
	else if (opcode == 0xc5)//push bc
	{
		writeToAddress(sp - 1, b);
		writeToAddress(sp - 2, c);
		sp -= 2;
		pc++;
		cycles = 16;
	}
	else if (opcode == 0x17)//rla
	{
		bool bit7 = (a & 0x80) != 0;
		a = (a << 1);
		if (carryStatus())
		{
			a = (a | 0x01);
		}
		setCarry(bit7);
		setZero(false);
		setSubtract(false);
		setHalf(false);
		pc++;
		cycles = 4;
	}

	else if (opcode == 0xd5)//push de
	{
		writeToAddress(sp - 1, d);
		writeToAddress(sp - 2, e);
		sp -= 2;
		pc++;
		cycles = 16;
	}
	else if (opcode == 0xe5)//push hl
	{
		writeToAddress(sp - 1, h);
		writeToAddress(sp - 2, l);
		sp -= 2;
		pc++;
		cycles = 16;
	}
	else if (opcode == 0xf5)//push af
	{
		f &= 0xf0;
		writeToAddress(sp - 1, a);
		writeToAddress(sp - 2, f);
		sp -= 2;
		pc++;
		cycles = 16;
	}
	else if (opcode == 0xc1)// pop bc
	{
		b = readFromAddress(sp + 1);
		c = readFromAddress(sp);
		sp += 2;
		pc++;
		cycles = 12;
	}
	else if (opcode == 0xd1)// pop de
	{
		d = readFromAddress(sp + 1);
		e = readFromAddress(sp);
		sp += 2;
		pc++;
		cycles = 12;
	}
	else if (opcode == 0xe1)// pop hl
	{
		h = readFromAddress(sp + 1);
		l = readFromAddress(sp);
		sp += 2;
		pc++;
		cycles = 12;
	}
	else if (opcode == 0xf1)// pop af
	{
		a = readFromAddress(sp + 1);
		f = readFromAddress(sp);
		f &= 0xf0;//bits 0-3 are always 0
		sp += 2;
		pc++;
		cycles = 12;
	}

	else if (opcode == 0x35)//dec (hl)
	{
		uint8_t temp = readFromAddress(concat(h, l));
		dec8(temp);
		writeToAddress(concat(h, l), temp);
		cycles = 12;
	}
	else if (opcode == 0x05)//dec b
	{
		dec8(b);
	}
	else if (opcode == 0x2d)//dec l
	{
		dec8(l);
	}
	else if (opcode == 0x3d)//dec a
	{
		dec8(a);
	}
	else if (opcode == 0x0d)//dec c
	{
		dec8(c);
	}
	else if (opcode == 0x15)//dec d
	{
		dec8(d);
	}
	else if (opcode == 0x25)//dec h
	{
		dec8(h);
	}
	else if (opcode == 0x1d)//dec e
	{
		dec8(e);
	}

	else if (opcode == 0x22)//ldi (hl),a
	{
		writeToAddress(concat(h, l), a);
		writePair(h, l, concat(h, l) + 1);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x04)//inc b
	{
		inc8(b);
	}
	else if (opcode == 0x0c)//inc c
	{
		inc8(c);
	}
	else if (opcode == 0x2c)//inc l
	{
		inc8(l);
	}
	else if (opcode == 0x24)//inc h
	{
		inc8(h);
	}
	else if (opcode == 0x1c)//inc e
	{
		inc8(e);
	}
	else if (opcode == 0x14)//inc d
	{
		inc8(d);
	}
	else if (opcode == 0x3c)//inc a
	{
		inc8(a);
	}
	else if (opcode == 0x03)//inc bc
	{
		writePair(b, c, concat(b, c) + 1);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x13)//inc de
	{
		writePair(d, e, concat(d, e) + 1);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x23)//inc hl
	{
		writePair(h, l, concat(h, l) + 1);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x34)//inc (hl)
	{
		uint16_t hl = concat(h, l);
		uint8_t temp = readFromAddress(hl);
		inc8(temp);
		writeToAddress(hl, temp);
		cycles = 12;
	}
	else if (opcode == 0x33)//inc sp
	{
		sp++;
		pc++;
		cycles = 8;
	}
	else if (opcode == 0xfe)//cp n
	{
		uint8_t temporary = a;
		sub8(a, readFromAddress(address + 1));
		a = temporary;
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x08)//ld (nn), sp
	{
		uint8_t high = (sp & 0xff00) >> 8;
		uint8_t low = (sp & 0x00ff);
		uint16_t target = concat(readFromAddress(address + 2), readFromAddress(address + 1));
		writeToAddress(target, low);
		writeToAddress(target + 1, high);
		cycles = 20;
		pc += 3;
	}
	else if (opcode == 0xea)//ld (nn), a
	{
		uint16_t next = concat(readFromAddress(address + 2), readFromAddress(address + 1));
		writeToAddress(next, a);
		pc += 3;
		cycles = 16;
	}

	else if (opcode == 0x2e)//ld l, n
	{
		l = readFromAddress(address + 1);
		pc += 2;
		cycles = 8;
	}
	else if (opcode == 0x1e)//ld e, n
	{
		e = readFromAddress(address + 1);
		pc += 2;
		cycles = 8;
	}
	else if (opcode == 0x47)//ld b,a
	{
		b = a;
		pc++;
		cycles = 4;
	}
	else if (opcode == 0x2a)//ld a, (hl+)
	{
		a = readFromAddress(concat(h, l));
		writePair(h, l, concat(h, l) + 1);
		cycles = 8;
		pc++;
	}
	else if (opcode == 0x3a)//ld a, (hl-)
	{
		a = readFromAddress(concat(h, l));
		writePair(h, l, concat(h, l) - 1);
		cycles = 8;
		pc++;
	}
	else if (opcode == 0xf0)//ld a, (0xff00+n)
	{
		a = readFromAddress(0xff00 + readFromAddress(address + 1));
		pc += 2;
		cycles = 12;
	}
	else if (opcode == 0x12)//ld (de), a
	{
		writeToAddress(concat(d, e), a);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x78)//ld a, b
	{
		a = b;
		pc++;
		cycles = 4;
	}
	else if (opcode == 0x60)//ld h,b
	{
		h = b;
		pc++;
		cycles = 4;
	}
	else if (opcode == 0x2f)//cpl
	{
		setSubtract(true);
		setHalf(true);
		a = ~a;
		cycles = 4;
		pc++;
	}
	else if (opcode == 0x59)//ld e, c
	{
		e = c;
		pc++;
		cycles = 4;
	}
	else if (opcode == 0x01)//ld bc, nn
	{
		b = readFromAddress(address + 2);//read n into b
		c = readFromAddress(address + 1);//read n into c
		pc += 3;
		cycles = 12;
	}
	else if (opcode == 0x0b)//dec bc
	{
		writePair(b, c, concat(b, c) - 1);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x1b)//dec de
	{
		writePair(d, e, concat(d, e) - 1);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x2b)//dec hl
	{
		writePair(h, l, concat(h, l) - 1);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x3b)//dec sp
	{
		sp--;
		pc++;
		cycles = 8;
	}
	else if (opcode == 0x7c)//ld a, h
	{
		a = h;
		pc++;
		cycles = 4;
	}
	else if (opcode == 0x7d)//ld a,l
	{
		a = l;
		pc++;
		cycles = 4;
	}
	else if (opcode == 0x26)//ld h, n
	{
		h = readFromAddress(address + 1);
		cycles = 8;
		pc += 2;
	}
	else if (opcode == 0x0f)//rrca
	{
		bool bit0 = (a & 0x01) != 0;
		a = (a >> 1);
		setCarry(false);
		if (bit0)
		{
			setCarry(true);
			a |= 0x80;
		}
		setZero(false);
		setSubtract(false);
		setHalf(false);
		cycles = 4;
		pc++;
	}
	else if (opcode == 0x1f)//rra (fast rr a)
	{
		uint8_t firstBit = ((a & 0x01));
		a = (a >> 1);
		if (carryStatus())
		{
			a = (a | 0x80);
		}
		else {
			a = (a & 0x7f);
		}
		setCarry(firstBit == 0x01);
		setHalf(false);
		setSubtract(false);
		setZero(false);
		pc++;
		cycles = 4;
	}
	else if (opcode == 0xce)//adc n
	{
		
		uint8_t next = readFromAddress(address + 1);
		if (carryStatus())
		{
			setHalf(((a & 0xf) + (next & 0xf) + 1) > 0x0f);
			setCarry(((unsigned long)a) + ((unsigned long)next) + 1 > 0xFF);

			a += next;
			a++;
		}
		else
		{
			setHalf(((a & 0xf) + (next & 0xf)) > 0x0f);
			setCarry(((unsigned long)a) + ((unsigned long)next) > 0xFF);

			a += next;
		}
		

		if (a == 0x00)
		{
			setZero(true);
		}
		else
		{
			setZero(false);
		}
		setSubtract(false);


		pc+=2;//because memory read
		cycles = 8;
		
	}
	else if (opcode == 0x37)//scf
	{
		setCarry(true);
		setHalf(false);
		setSubtract(false);
		cycles = 4;
		pc++;
	}
	else if (opcode == 0xf8)//ld hl, sp+n
	{
		int8_t offset = readFromAddress(address + 1);
		writePair(h, l, sp + offset);
		cycles = 12;
		pc += 2;
		if (offset >= 0)
		{
			setCarry(((sp & 0xff) + (offset)) > 0xff);
			setHalf(((sp & 0xf) + (offset & 0xf)) > 0xf);
		}
		else
		{
			setCarry(((sp + offset) & 0xff) <= (sp & 0xff));
			setHalf(((sp + offset) & 0xf) <= (sp & 0xf));
		}
		setSubtract(false);
		setZero(false);
	}
	else if (opcode == 0xf9)//ld sp, hl
	{
		sp = concat(h, l);
		cycles = 8;
		pc++;
	}
	else if (opcode == 0x27)//daa
	{
		if (!subtractStatus())
		{
			if (carryStatus() || a > 0x99)
			{
				a += 0x60;
				setCarry(true);
			}
			if (halfStatus() || (a & 0x0f) > 0x09)
			{
				a += 0x06;
			}
		}
		else {
			if (carryStatus()) { a -= 0x60; }
			if (halfStatus()) { a -= 0x06; }
		}
		setZero(a == 0);
		setHalf(false);
		cycles = 4;
		pc++;
	}
	else if (opcode == 0x07)//rlca
	{
		uint8_t firstBit = ((a & 0x80));
		a = (a << 1);
		a = a | (firstBit >> 7);
		setCarry(firstBit == 0x80);
		setHalf(false);
		setSubtract(false);
		setZero(false);
		cycles = 4;
		pc++;
	}
	else if (opcode == 0x3f)//ccf
	{
		setSubtract(false);
		setHalf(false);
		setCarry(!carryStatus());
		pc++;
		cycles = 4;
	}
	else if (opcode == 0x02)//ld (bc), a
	{
		writeToAddress(concat(b, c), a);
		pc++;
		cycles = 8;
	}
	else if (opcode == 0xcb)//prefix for extended instructions
	{
		executePrefixedInstruction(readFromAddress(address + 1));
	}
	else {
		if (!executeStructural(opcode, address))
		{
			errorAddress = -1;
			logger::logError("Opcode not implemented", address, opcode);
			throw exceptions::invalidOpcode("OPCODE NOT IMPLEMENTED", address, opcode);
			
		}
	}

	processTimer(cycles);
	checkInterrupts();

}




bool executeStructural(uint8_t opcode, uint16_t address)
{

	if (opcode == 0x10)//stop
	{
		logger::logError("Encountered stop opcode!", address, opcode);
		throw exceptions::invalidOpcode("ERROR: Encountered stop opcode! (0x10)", address, opcode);
	}
	else if (opcode == 0xcd)//call nn
	{
		pc += 3;
		uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
		uint8_t low = pc & 0x00ff;
		writeToAddress(sp - 1, high);
		writeToAddress(sp - 2, low);
		pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
		sp -= 2;
		cycles = 24;
	}
	else if (opcode == 0xcc)//call z, nn
	{
		pc += 3;
		if (zeroStatus())
		{
			uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
			uint8_t low = pc & 0x00ff;
			writeToAddress(sp - 1, high);
			writeToAddress(sp - 2, low);
			pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
			sp -= 2;
			cycles = 24;
		}
		else
		{
			cycles = 12;
		}

	}
	else if (opcode == 0xc4)//call nz, nn
	{
		pc += 3;
		if (!zeroStatus())
		{
			uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
			uint8_t low = pc & 0x00ff;
			writeToAddress(sp - 1, high);
			writeToAddress(sp - 2, low);
			pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
			sp -= 2;
			cycles = 24;
		}
		else
		{
			cycles = 12;
		}
	}
	else if (opcode == 0xd4)//call nc, nn
	{
		pc += 3;
		if (!carryStatus())
		{
			uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
			uint8_t low = pc & 0x00ff;
			writeToAddress(sp - 1, high);
			writeToAddress(sp - 2, low);
			pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
			sp -= 2;
			cycles = 24;
		}
		else
		{
			cycles = 12;
		}

	}
	else if (opcode == 0xdc)//call c, nn
	{
		pc += 3;
		if (carryStatus())
		{
			uint8_t high = (((uint16_t)pc & 0xff00) >> 8);
			uint8_t low = pc & 0x00ff;
			writeToAddress(sp - 1, high);
			writeToAddress(sp - 2, low);
			pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
			sp -= 2;
			cycles = 24;
		}

		else
		{
			cycles = 12;
		}

	}
	else if (opcode == 0xc9)//ret
	{
		pc = concat(readFromAddress(sp + 1), readFromAddress(sp));
		sp += 2;
		cycles = 16;
	}
	else if (opcode == 0xd0)//ret nc
	{
		if (!carryStatus())
		{
			pc = concat(readFromAddress(sp + 1), readFromAddress(sp));
			sp += 2;
			cycles = 20;
		}
		else
		{
			pc++;
			cycles = 8;
		}
	}
	else if (opcode == 0xc0)//ret nz
	{
		if (!zeroStatus())
		{
			pc = concat(readFromAddress(sp + 1), readFromAddress(sp));
			sp += 2;
			cycles = 20;
		}
		else
		{
			pc++;
			cycles = 8;
		}
	}
	else if (opcode == 0xc8)//ret z
	{
		if (zeroStatus())
		{
			pc = concat(readFromAddress(sp + 1), readFromAddress(sp));
			sp += 2;
			cycles = 20;
		}
		else
		{
			pc++;
			cycles = 8;
		}
	}
	else if (opcode == 0xd8)//ret c
	{
		if (carryStatus())
		{
			pc = concat(readFromAddress(sp + 1), readFromAddress(sp));
			sp += 2;
			cycles = 20;
		}
		else
		{
			pc++;
			cycles = 8;
		}
	}
	else if (opcode == 0x28)//jr z,n
	{
		pc += 2;
		if (zeroStatus()) {
			pc += (int8_t)readFromAddress(address + 1);
			cycles = 12;
		}
		else {
			cycles = 8;
		}
	}
	else if (opcode == 0x20)//jr nz, n
	{
		pc += 2;//increment to next instruction
		if (!zeroStatus())
		{
			pc += (int8_t)readFromAddress(address + 1);//jump
			cycles = 12;
		}
		else { //if zero bit is set
			cycles = 8;
		}
	}
	else if (opcode == 0x38)//jr c, n
	{
		pc += 2;
		if (carryStatus())
		{
			pc += (int8_t)readFromAddress(address + 1);
			cycles = 12;
		}
		else {
			cycles = 8;
		}
	}
	else if (opcode == 0x30)//jr nc, n
	{
		pc += 2;
		if (!carryStatus()) {
			pc += (int8_t)readFromAddress(address + 1);
			cycles = 12;
		}
		else {
			cycles = 8;
		}
	}
	else if (opcode == 0x18)//jr n
	{
		pc += 2;
		pc += (int8_t)readFromAddress(address + 1);
		cycles = 12;
	}
	else if (opcode == 0xc3)//jp nn
	{
		pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
		cycles = 16;
		if (pc == 0x0000)
		{
			logger::logWarning("Jumped to 0x0000, probably a bug.", address, opcode);
			throw exceptions::invalidInstruction("Jumped to 0, probably a bug");
		}
	}
	else if (opcode == 0xca)//jp z, nn
	{
		if (zeroStatus())
		{
			pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
			cycles = 16;
		}
		else {
			pc += 3;
			cycles = 12;
		}
	}
	else if (opcode == 0xda)//jp c, nn
	{
		if (carryStatus())
		{
			pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
			cycles = 16;
		}
		else {
			pc += 3;
			cycles = 12;
		}
	}
	else if (opcode == 0xc2)//jp nz, nn
	{
		if (!zeroStatus())
		{
			pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
			cycles = 16;
		}
		else {
			pc += 3;
			cycles = 12;
		}
	}
	else if (opcode == 0xd2)//jp nc, nn
	{
		if (!carryStatus())
		{
			pc = concat(readFromAddress(address + 2), readFromAddress(address + 1));
			cycles = 16;
		}
		else {
			pc += 3;
			cycles = 12;
		}
	}
	else if (opcode == 0xf3)//di(disable interrupts)
	{
		enableInterrupts = false;
		pc++;
		cycles = 4;
	}
	else if (opcode == 0xd9)//reti
	{
		enableInterrupts = true;
		pc = concat(readFromAddress(sp + 1), readFromAddress(sp));
		sp += 2;
		cycles = 16;
	}
	else if (opcode == 0xfb)//ei (enable interrupts)
	{
		enableInterrupts = true;
		pc++;
		cycles = 4;
	}
	else if (opcode == 0xe9)//jp hl
	{
		pc = concat(h, l);
		cycles = 4;
	}
	else if (opcode == 0xc7)//rst 0x00
	{
		pc++;//?
		writeToAddress(sp - 1, (uint8_t)(pc >> 8));//write PCh to stack
		writeToAddress(sp - 2, (uint8_t)(pc & 0x00ff));//write PCl to stack
		sp -= 2;
		cycles = 16;
		pc = 0x0000;
	}
	else if (opcode == 0xd7)//rst 0x10
	{
		pc++;//?
		writeToAddress(sp - 1, (uint8_t)(pc >> 8));//write PCh to stack
		writeToAddress(sp - 2, (uint8_t)(pc & 0x00ff));//write PCl to stack
		sp -= 2;
		cycles = 16;
		pc = 0x0010;
	}
	else if (opcode == 0xe7)//rst 0x20
	{
		pc++;//?
		writeToAddress(sp - 1, (uint8_t)(pc >> 8));//write PCh to stack
		writeToAddress(sp - 2, (uint8_t)(pc & 0x00ff));//write PCl to stack
		sp -= 2;
		cycles = 16;
		pc = 0x0020;
	}
	else if (opcode == 0xf7)//rst 0x30
	{
		pc++;//?
		writeToAddress(sp - 1, (uint8_t)(pc >> 8));//write PCh to stack
		writeToAddress(sp - 2, (uint8_t)(pc & 0x00ff));//write PCl to stack
		sp -= 2;
		cycles = 16;
		pc = 0x0030;
	}
	else if (opcode == 0xcf)//rst 0x08
	{
		pc++;//?
		writeToAddress(sp - 1, (uint8_t)(pc >> 8));//write PCh to stack
		writeToAddress(sp - 2, (uint8_t)(pc & 0x00ff));//write PCl to stack
		sp -= 2;
		cycles = 16;
		pc = 0x0008;
	}
	else if (opcode == 0xdf)//rst 0x18
	{
		pc++;//?
		writeToAddress(sp - 1, (uint8_t)(pc >> 8));//write PCh to stack
		writeToAddress(sp - 2, (uint8_t)(pc & 0x00ff));//write PCl to stack
		sp -= 2;
		cycles = 16;
		pc = 0x0018;
	}
	else if (opcode == 0xef)//rst 0x28
	{
		pc++;//?
		writeToAddress(sp - 1, (uint8_t)(pc >> 8));//write PCh to stack
		writeToAddress(sp - 2, (uint8_t)(pc & 0x00ff));//write PCl to stack
		sp -= 2;
		cycles = 16;
		pc = 0x0028;
	}
	else if (opcode == 0xff)//rst 0x38
	{
	    logger::logWarning("RST 0x38 instruction encountered. Might be an error.", address, opcode);
		pc++;//?
		writeToAddress(sp - 1, (uint8_t)(pc >> 8));//write PCh to stack
		writeToAddress(sp - 2, (uint8_t)(pc & 0x00ff));//write PCl to stack
		sp -= 2;
		cycles = 16;
		pc = 0x0038;
	}
	else if (opcode == 0x76)//halt
	{
		cycles = 4;
		pc++;
		halted = true;
		if (LOG_VERBOSE) {
			logger::logWarningNoData("HALT bug is not implemented.");
		}
	}
	else //Unimplemented opcode
	{
		return false;
	}
	return true;
}
