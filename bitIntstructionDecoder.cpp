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
#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by andrew on 11/25/19.
//
#include <iostream>
#include "interpreter.h"
#include "lcdController.h"

using namespace std;
void executePrefixedInstruction(uint8_t opcode)
{
    uint8_t targetRegister = opcode & 0x07;     //bottom 3 bits are which register (b,c,d,e,h,l,(hl),a
    uint8_t instruction = (opcode & 0xf8) >>3;   //top 5 bits are instruction (rlc,rrc,rl,rr,sla,sra,swap,srl,bit 0-7,res 0-7,set 0-7)

    pc+=2;
    cycles = 8;
    if (targetRegister == 0x06)//(hl)
        cycles = 16;
    if (targetRegister == 0x06 && (opcode >= 0x40 && opcode <=0x7f))//hl and bit instruction
        cycles = 12;

    bool memoryReference = false;
    uint8_t temp;
    uint8_t* reg;
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
                reg = &a;
                throw "Error in bit instruction, invalid target register";
        }
    }
    else //read from address in hl
    {
        temp = readFromAddress(concat(h,l));
        reg = &temp;
    }

    //reg now has a pointer to the register, or to temp if a memory reference
    //the bool memoryReference determines which it is

    if (opcode >= 0x40 && opcode <=0x7f)//bit instructions
    {
        uint8_t mask = 0x01 << (instruction-0x08);
        setZero(((*reg) & mask) != mask);
        setHalf(true);
        setSubtract(false);
    }
    else if (opcode >= 0x30 && opcode <=0x37)//swap
    {

        uint8_t front = (*reg & 0x0f)<<4;
        uint8_t back = (*reg & 0xf0)>>4;
        *reg = (front | back);
        setCarry(false);
        setHalf(false);
        setSubtract(false);
        setZero(*reg == 0x00);
        if (memoryReference)
        {
            writeToAddress(concat(h,l), *reg);//if referencing hl, write the swapped temp back to the address
        }
    }
    else if (opcode <= 0x07)//rlc
    {
        bool oneLeaving = TestBit(*reg, 7);
        setCarry(oneLeaving);
        *reg = (*reg)<<1;
        if (oneLeaving)
        {
            *reg = (*reg) | 0x01;
        }
        if (memoryReference)
        {
            writeToAddress(concat(h,l),*reg);
        }
        setZero(*reg == 0x00);
        setSubtract(false);
        setHalf(false);
    }
    else if (opcode >= 0x08 && opcode <=0x0f)//rrc
    {
        bool oneLeaving = TestBit(*reg, 0);
        setCarry(oneLeaving);
        *reg = (*reg)>>1;
        if (oneLeaving)
        {
            *reg = (*reg) | 0x80;
        }
        if (memoryReference)
        {
            writeToAddress(concat(h,l),*reg);
        }
        setZero(*reg == 0x00);
        setSubtract(false);
        setHalf(false);
    }
    else if (opcode >=0x10 && opcode <= 0x17)//rl
    {
        bool oneLeaving = TestBit(*reg, 7);
        *reg = (*reg)<<1;
        if (carryStatus())
        {
            *reg = (*reg) | 0x01;
        }
        setCarry(oneLeaving);
        if (memoryReference)
        {
            writeToAddress(concat(h,l),*reg);
        }
        setZero(*reg == 0x00);
        setSubtract(false);
        setHalf(false);
    }
    else if (opcode >= 0x18 && opcode <= 0x1f)//rr
    {
        bool oneLeaving = TestBit(*reg, 0);
        *reg = (*reg)>>1;
        if (carryStatus())
        {
            *reg = (*reg) | 0x80;
        }
        setCarry(oneLeaving);
        if (memoryReference)
        {
            writeToAddress(concat(h,l),*reg);
        }
        setZero(*reg == 0x00);
        setSubtract(false);
        setHalf(false);
    }
    else if (opcode >= 0x20 && opcode <= 0x27)//sla
    {
        setCarry(TestBit(*reg, 7));
        *reg = (*reg)<<1;
        setZero(*reg == 0x00);
        setSubtract(false);
        setHalf(false);
		if (memoryReference)
		{
			writeToAddress(concat(h, l), *reg);
		}
    }
    else if (opcode >= 0x28 && opcode <= 0x2f)//sra
    {
        setCarry(TestBit(*reg, 0));
        *reg = (uint8_t)(((int8_t )(*reg))>>1);
        setZero(*reg == 0x00);
        setSubtract(false);
        setHalf(false);
		if (memoryReference)
		{
			writeToAddress(concat(h, l), *reg);
		}
        //cout<<"WARNING: Arithmetic right shift is untested.\n";
    }
    else if (opcode >= 0x38 && opcode <= 0x3f)//srl
    {
        setCarry(TestBit(*reg, 0));
        *reg = (*reg)>>1;
        setZero(*reg == 0x00);
        setSubtract(false);
        setHalf(false);
		if (memoryReference)
		{
			writeToAddress(concat(h, l), *reg);
		}
    }
    else if (opcode >= 0x80 && opcode <= 0xbf)//res
    {
        *reg = BitReset(*reg, instruction-0x10);
        if (memoryReference)
        {
            writeToAddress(concat(h,l),*reg);
        }
    }
    else if (opcode >= 0xc0)//set
    {
        *reg = BitSet(*reg, instruction-0x18);
        if (memoryReference)
        {
            writeToAddress(concat(h,l),*reg);
        }
    }
    else
    {
        errorAddress = -1;
        cout<<"Suffix: 0x"<<hex<<(uint16_t)(opcode)<<dec<<endl;
        throw "PREFIXED OPCODE NOT IMPLEMENTED";
    }
}
#pragma clang diagnostic pop