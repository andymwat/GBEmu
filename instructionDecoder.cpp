//
// Created by andrew on 11/20/19.
//
#pragma ide diagnostic ignored "hicpp-exception-baseclass"
#pragma ide diagnostic ignored "readability-static-accessed-through-instance"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
//
// Created by andrew on 11/17/19.
//
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "cartridge.h"
#include "interpreter.h"
#include "lcdController.h"
#include "keyboardInput.h"

using namespace std;

void execute(uint16_t address)
{

    if (halted)
    {
        cycles = 4;
        processTimer(cycles);
        checkInterrupts();
        return;
    }
    uint8_t opcode = readFromAddress(address);
    //std::cout<<"Address/opcode: " <<hex<<"\t0x"<<address<<"\t0x"<<(uint16_t)opcode<<dec<<std::endl;

    uint8_t targetRegister = opcode & 0x07;     //bottom 3 bits are which register (b,c,d,e,h,l,(hl),a
    uint8_t instruction = (opcode & 0xf8) >>3;   //top 5 bits are instruction (rlc,rrc,rl,rr,sla,sra,swap,srl,bit 0-7,res 0-7,set 0-7)
    bool memoryReference = false;
    uint8_t temp;
    uint8_t* reg;





    if (opcode >=0x40 && opcode <= 0xbf && opcode != 0x76)//loads, add, adc, sub, sbc, and, xor, or, cp
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
                    throw "Error in middle chunk of instructions, invalid target register";
            }
        }
        else //read from address in hl
        {
            temp = readFromAddress(concat(h,l));
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
        }

        *dest = *reg;

        if (memoryTarget)
        {
            if (memoryReference)
            {
                throw "You done messed up.";
            }
            writeToAddress(concat(h,l),*dest);
        }

        pc++;
        cycles = 4;
        if (memoryReference || memoryTarget)
            cycles = 8;
    }
    else if (opcode >= 0x80 && opcode <= 0x87)//add a, x
    {
        add8(a,*reg);
        if (memoryReference)
        {
            cycles = 8;
        }
    }
    else if (opcode >= 0x88 && opcode <= 0x8f)//adc a, x
    {
        if (carryStatus())
            a++;
        add8(a,*reg);
        if (memoryReference)
            cycles = 8;
    }
    else if (opcode >=0x90 && opcode <= 0x97)//sub a, x
    {
        sub8(a, *reg);
        if (memoryReference)
            cycles = 8;
    }
    else if (opcode >=0x98 && opcode <= 0x9f)//sbc a, x
    {
        if (carryStatus())
            a--;
        sub8(a,*reg);
        if (memoryReference)
            cycles = 8;
    }
    else if (opcode >= 0xa0 && opcode <= 0xa7)//and a, x
    {
        a = a & (*reg);
        setZero(a==0x00);
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
        setZero(a==0x00);
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
        uint8_t temporary = a;
        sub8(a,*reg);
        a = temporary;
        if (memoryReference)
            cycles = 8;
    }
    else if (opcode == 0x31)//ld sp, nn
    {
        sp = concat(readFromAddress(address+2),readFromAddress(address+1));//read nn into sp
        pc+=3;
        cycles = 12;
        // cout<<"Loaded " << to_string(sp) <<" into sp."<<endl;
    }
    else if (opcode == 0x00)//nop
    {
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x10)//stop
    {
        throw "ERROR: Encountered stop opcode! (0x10)";
    }
    /*
     *
    else if (opcode == 0xb0)//or a, b
    {
        or8(a,b);
    }
    else if (opcode == 0xb1)//or a, c
    {
        or8(a,c);
    }
    else if (opcode == 0xb5)//or a, l
    {
        or8(a,l);
    }
    else if (opcode == 0xb6)//or a, (hl)
    {
        or8(a,readFromAddress(concat(h,l)));
        cycles = 8;
    }
    else if (opcode == 0xaf)//xor a, a
    {
        a = 0x00;
        setCarry(false);
        setSubtract(false);
        setHalf(false);
        setZero(true);
        pc++;
        cycles = 4;
    }
    else if (opcode == 0xa9)//xor a, c
    {
        a = a ^ c;
        setCarry(false);
        setSubtract(false);
        setHalf(false);
        setZero(a==0x00);
        pc++;
        cycles = 4;
    }

    else if (opcode == 0xae)//xor a, (hl)
    {
        a = a ^ readFromAddress(concat(h,l));
        setCarry(false);
        setSubtract(false);
        setHalf(false);
        setZero(a==0x00);
        cycles = 8;
        pc++;
    }
    else if (opcode == 0xb7)//or a, a, no effect
    {
        or8(a,a);
    }
    else if (opcode == 0x91)//sub a, c
    {
        sub8(a,c);
    }
    else if (opcode == 0x6e)//ld l, (hl)
    {
        l = readFromAddress(concat(h,l));
        cycles = 8;
        pc++;
    }
    else if (opcode == 0x77)//ld (hl), a
    {
        writeToAddress(concat(h,l), a);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x72)//ld (hl), d
    {
        writeToAddress(concat(h,l), d);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x71)//ld (hl), c
    {
        writeToAddress(concat(h,l), c);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x70)//ld (hl), b
    {
        writeToAddress(concat(h,l), b);
        pc++;
        cycles = 8;
    }

    else if (opcode == 0xa0)//and b
    {
        a = a & b;
        setZero(a==0x00);
        setSubtract(false);
        setHalf(true);
        setCarry(false);
        pc++;
        cycles = 4;
    }
    else if (opcode == 0xa1)//and c
    {
        a = a & c;
        setZero(a==0x00);
        setSubtract(false);
        setHalf(true);
        setCarry(false);
        pc++;
        cycles = 4;
    }
    else if (opcode == 0xa7)//and a
    {
        setZero(a==0x00);
        setSubtract(false);
        setHalf(true);
        setCarry(false);
        pc++;
        cycles = 4;
    }

    else if (opcode == 0xbb)//cp a,e
    {
        setZero(a==e);
        setHalf((int)(a & 0xf) - (int)(e&0xf) < 0);
        setCarry(a<e);
        setSubtract(true);
        cycles = 4;
        pc++;
    }
     */
    else if (opcode == 0xee)//xor a, n
    {
        a = a ^ readFromAddress(address+1);
        setCarry(false);
        setSubtract(false);
        setHalf(false);
        setZero(a==0x00);
        pc+=2;
        cycles = 8;
    }
    else if (opcode == 0xf6)//or a, n
    {
        a = a | readFromAddress(address+1);
        setCarry(false);
        setSubtract(false);
        setHalf(false);
        setZero(a==0x00);
        pc+=2;
        cycles = 8;
    }
    /*
    else if (opcode == 0x80)//add a, b
        add8(a,b);
    else if (opcode == 0x81)//add a, c
        add8(a,c);
    else if (opcode == 0x82)//add a, d
        add8(a,d);
    else if (opcode == 0x83)//add a, e
        add8(a,e);
    else if (opcode == 0x84)//add a, h
        add8(a,h);
    else if (opcode == 0x85)//add a, l
        add8(a,l);
    else if (opcode == 0x87)//add a, a
        add8(a,a);
    */
    else if (opcode == 0xc6)//add a, n
    {
        add8(a,readFromAddress(address+1));
        cycles = 8;
        pc++; //needed so that final pc is +=2
    }
    else if (opcode == 0x29)//add hl, hl
    {
        uint16_t result = concat(h,l);
        add16(result, result);
        writePair(h,l,result);
    }
    else if (opcode == 0x09)//add hl, bc
    {
        uint16_t result = concat(h,l);
        add16(result, concat(b,c));
        writePair(h,l,result);
    }
    else if (opcode == 0x19)//add hl, de
    {
        uint16_t result = concat(h,l);
        add16(result, concat(d,e));
        writePair(h,l,result);
    }
    else if (opcode == 0x39)//add hl, sp
    {
        uint16_t temporary = concat(h,l);
        add16(temporary, sp);
        writePair(h,l,temporary);
    }
    else if (opcode == 0xd6)//sub a, n
    {
        sub8(a,readFromAddress(address+1));
        cycles = 8;
        pc++; //needed so that final pc is +=2
    }
    else if (opcode == 0x21)//ld hl, nn
    {
        l = readFromAddress(address+1);//read n into l
        h = readFromAddress(address+2);//read n into h
        pc+=3;
        cycles = 12;
        //cout<<"Loaded " << to_string(l) << " and " << to_string(h) <<" into l and h, respectively."<<endl;
    }
    else if (opcode == 0x32)//ldd (hl),a
    {
        //cout<<"Writing a into address 0x" <<hex<< (concat(h,l))<<dec <<"."<<endl;
        writeToAddress(concat(h,l), a);
        writePair(h,l,concat(h,l)-1);
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
        writeToAddress(concat(h,l),readFromAddress(address+1));
        pc+=2;
        cycles = 12;
    }
    else if (opcode == 0xe0)//ld (0xff00+n), a
    {
        writeToAddress(0xff00 + readFromAddress(address+1),a);
        pc+=2;
        cycles = 12;
    }
    else if (opcode == 0x11) //ld de, nn
    {
        d = readFromAddress(address+2);//read n into d
        e = readFromAddress(address+1);//read n into e
        pc+=3;
        cycles = 12;
    }
    else if (opcode ==0x1a)//ld a, (de)
    {
        a = (readFromAddress(concat(d,e)));
        pc++;
        cycles = 8;
    }
    else if (opcode ==0x0a)//ld a, (bc)
    {
        a = (readFromAddress(concat(b,c)));
        pc++;
        cycles = 8;
    }
    else if (opcode == 0xe8)//add sp, i8
    {
        cout<<"WARNING: Carry/half-carry with \"add sp, i8\" is untested.\n";
        int8_t next = readFromAddress(address+1);
        setCarry((unsigned int)sp + (signed int)next> 0xffff   || (((unsigned int)sp&0xffff) + ((signed int)next&0xffff))>0xffff);
        setHalf(((sp & 0x0f) + (next& 0x0f)) > 0xf || ((sp & 0x0f00) + (next& 0x0f00)) > 0x0f00);
        setSubtract(false);
        setZero(false);
        sp += next;
        cycles = 16;
        pc+=2;
    }
    else if (opcode == 0xde)//sbc a, u8
    {
        if (carryStatus())
        {
            a--;
        }
        sub8(a, readFromAddress(address+1));
        //if carry is 0, same as sub a, u8
        cycles = 8;
        pc++;//memory read
    }
    /*
    else if (opcode == 0x67)//ld h, a
    {
        h=a;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x57)//ld d, a
    {
        d=a;
        pc++;
        cycles = 4;
    }
    else if (opcode ==0x4e)//ld c, (hl)
    {
        c = (readFromAddress(concat(h,l)));
        pc++;
        cycles = 8;
    }
    else if (opcode ==0x5e)//ld e, (hl)
    {
        e = (readFromAddress(concat(h,l)));
        pc++;
        cycles = 8;
    }
    else if (opcode ==0x56)//ld d, (hl)
    {
        d = (readFromAddress(concat(h,l)));
        pc++;
        cycles = 8;
    }
    else if (opcode ==0x46)//ld b, (hl)
    {
        b = (readFromAddress(concat(h,l)));
        pc++;
        cycles = 8;
    }
    else if (opcode ==0x7e)//ld a, (hl)
    {
        a = (readFromAddress(concat(h,l)));
        pc++;
        cycles = 8;
    }
     */
    else if (opcode ==0xfa)//ld a, (nn)
    {
        a = (readFromAddress(concat(readFromAddress(address+2),readFromAddress(address+1))));
        pc+=3;
        cycles = 16;
    }
    else if (opcode == 0xe6)//and a, n
    {
        a = a & readFromAddress(address+1);
        setZero(a == 0x00);
        setSubtract(false);
        setHalf(true);
        setCarry(false);
        pc+=2;
        cycles = 8;
    }
    else if (opcode == 0xcd)//call nn
    {
        pc+=3;
        uint8_t high = (((uint16_t )pc & 0xff00)>>8);
        uint8_t low = pc & 0x00ff;
        writeToAddress(sp-1,high);
        writeToAddress(sp-2,low);
        pc = concat(readFromAddress(address+2),readFromAddress(address+1));
        sp-=2;
        cycles = 24;
    }
    else if (opcode == 0xcc)//call z, nn
    {
        pc+=3;
        if (zeroStatus())
        {
            uint8_t high = (((uint16_t )pc & 0xff00)>>8);
            uint8_t low = pc & 0x00ff;
            writeToAddress(sp-1,high);
            writeToAddress(sp-2,low);
            pc = concat(readFromAddress(address+2),readFromAddress(address+1));
            sp-=2;
            cycles = 24;
        }
        else
        {
            cycles = 12;
        }

    }
    else if (opcode == 0xc4)//call nz, nn
    {
        pc+=3;
        if (!zeroStatus())
        {
            uint8_t high = (((uint16_t )pc & 0xff00)>>8);
            uint8_t low = pc & 0x00ff;
            writeToAddress(sp-1,high);
            writeToAddress(sp-2,low);
            pc = concat(readFromAddress(address+2),readFromAddress(address+1));
            sp-=2;
            cycles = 24;
        }
        else
        {
            cycles = 12;
        }

    }
    else if (opcode == 0xd4)//call nc, nn
    {
        pc+=3;
        if (!carryStatus())
        {
            uint8_t high = (((uint16_t )pc & 0xff00)>>8);
            uint8_t low = pc & 0x00ff;
            writeToAddress(sp-1,high);
            writeToAddress(sp-2,low);
            pc = concat(readFromAddress(address+2),readFromAddress(address+1));
            sp-=2;
            cycles = 24;
        }
        else
        {
            cycles = 12;
        }

    }
    else if (opcode == 0xdc)//call c, nn
    {
        pc+=3;
        if (carryStatus())
        {
            uint8_t high = (((uint16_t )pc & 0xff00)>>8);
            uint8_t low = pc & 0x00ff;
            writeToAddress(sp-1,high);
            writeToAddress(sp-2,low);
            pc = concat(readFromAddress(address+2),readFromAddress(address+1));
            sp-=2;
            cycles = 24;
        }

        else
        {
            cycles = 12;
        }

    }
    else if (opcode == 0xc9)//ret
    {
        //cout<<"WARNING";
        //cout<<" returning to " << to_string(concat(readFromAddress(sp+1),readFromAddress(sp))) <<endl;
        //dumpRegisters();
        pc = concat (readFromAddress(sp+1),readFromAddress(sp));
        sp += 2;
        cycles = 16;
    }
    else if (opcode == 0xd0)//ret nc
    {
        if (!carryStatus())
        {
            pc = concat(readFromAddress(sp+1),readFromAddress(sp));
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
            pc = concat(readFromAddress(sp+1),readFromAddress(sp));
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
            pc = concat(readFromAddress(sp+1),readFromAddress(sp));
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
            pc = concat(readFromAddress(sp+1),readFromAddress(sp));
            sp += 2;
            cycles = 20;
        }
        else
        {
            pc++;
            cycles = 8;
        }
    }
    else if (opcode == 0x0e)//ld c,n
    {
        c = readFromAddress(address+1);
        pc+=2;
        cycles =8;
    }
    else if (opcode == 0x3e)//ld a,n
    {
        a = readFromAddress(address + 1);
        pc += 2;
        cycles = 8;
    }
    /*
    else if (opcode == 0x62)//ld h, d
    {
        h = d;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x4f)//ld c, a
    {
        c = a;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x6f)//ld l, a
    {
        l = a;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x6b) //ld l, e
    {
        l = e;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x79)//ld a, c
    {
        a = c;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x7a)//ld a, d
    {
        a = d;
        pc++;
        cycles = 4;
    }

    else if (opcode == 0x7b)//ld a,e
    {
        a=e;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x5f)//ld e,a
    {
        e=a;
        pc++;
        cycles = 4;
    }
     */
    else if (opcode ==0x06)//ld b, n
    {
        b = readFromAddress(address+1);
        pc+=2;
        cycles = 8;
    }

    else if (opcode == 0x16)//ld d, n
    {
        d = readFromAddress(address+1);
        pc+=2;
        cycles =8;
    }
    else if (opcode == 0xc5)//push bc
    {
        writeToAddress(sp-1,b);
        writeToAddress(sp-2,c);
        sp-=2;
        pc++;
        cycles = 16;
    }
    else if (opcode == 0x17)//rla
    {
        uint8_t firstBit = ((a | 0x80));
        a = (a<<1);
        if (carryStatus())
        {
            a = (a | 0x01);
        } else{
            a = (a & 0xfe);
        }
        setCarry(firstBit == 0x80);
        setZero(false);
        setSubtract(false);
        setHalf(false);
        pc++;
        cycles = 4;
    }

    else if (opcode == 0xd5)//push de
    {
        writeToAddress(sp-1,d);
        writeToAddress(sp-2,e);
        sp-=2;
        pc++;
        cycles = 16;
    }
    else if (opcode == 0xe5)//push hl
    {
        writeToAddress(sp-1,h);
        writeToAddress(sp-2,l);
        sp-=2;
        pc++;
        cycles = 16;
    }
    else if (opcode == 0xf5)//push af
    {
        /*
        f = 0;
        if (zeroStatus())
        {
            BitSet(f,7);
        }
        if (subtractStatus())
        {
            BitSet(f,6);
        }
        if (halfStatus())
        {
            BitSet(f,5);
        }
        if (carryStatus())
        {
            BitSet(f,4);
        }
         */
        f &= 0xf0;
        writeToAddress(sp-1,a);
        writeToAddress(sp-2,f);
        sp-=2;
        pc++;
        cycles = 16;
    }
    else if (opcode == 0xc1)// pop bc
    {
        b = readFromAddress(sp+1);
        c = readFromAddress(sp);
        sp+=2;
        pc++;
        cycles = 12;
    }
    else if (opcode == 0xd1)// pop de
    {
        d = readFromAddress(sp+1);
        e = readFromAddress(sp);
        sp+=2;
        pc++;
        cycles = 12;
    }
    else if (opcode == 0xe1)// pop hl
    {
        h = readFromAddress(sp+1);
        l = readFromAddress(sp);
        sp+=2;
        pc++;
        cycles = 12;
    }
    else if (opcode == 0xf1)// pop af
    {
        a = readFromAddress(sp+1);
        f = readFromAddress(sp);
        /*
        setZero((f&0x80) == 0x80);
        setSubtract((f&0x40)==0x40);
        setHalf((f&0x20)==0x20);
        setCarry((f&0x10)==0x10);
         */
        f &= 0xf0;//bits 0-3 are always 0
        sp+=2;
        pc++;
        cycles = 12;
    }
    else if (opcode == 0x35)//dec (hl)
    {
        uint8_t temp = readFromAddress(concat(h,l));
        dec8(temp);
        writeToAddress(concat(h,l),temp);
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
        writeToAddress(concat(h,l), a);
        writePair(h,l,concat(h,l)+1);
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
        writePair(b,c,concat(b,c)+1);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x13)//inc de
    {
        writePair(d,e,concat(d,e)+1);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x23)//inc hl
    {
        writePair(h,l,concat(h,l)+1);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x34)//inc (hl)
    {
        uint16_t hl = concat(h,l);
        uint8_t temp = readFromAddress(hl);
        inc8(temp);
        writeToAddress(hl, temp);
        //pc++; //because of pc in inc earlier
        //cout<<"DEBUG: Previously broken \"inc (hl)\" fixed.\n";
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
        sub8(a,readFromAddress(address+1));
        a=temporary;
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x08)//ld (nn), sp
    {
        uint8_t high = (sp&0xff00) >>8;
        uint8_t low = (sp&0x00ff);
        uint16_t target = concat(readFromAddress(address+2),readFromAddress(address+1));
        writeToAddress(target, low);
        writeToAddress(target+1, high);
        cycles = 20;
        pc+=3;
        //cout<<"WARNING: opcode 0x08 (ld (u16), sp) untested.\n";
    }
    else if (opcode == 0xea)//ld (nn), a
    {
        uint16_t next = concat(readFromAddress(address+2),readFromAddress(address+1));
        /* if (next ==  0xd65f && a == 0x0)
         {
             dumpWorkRamToFile("/home/andrew/Downloads/ramdump.bin");
             throw "Wrote 0 to 0xd65f, dumping workram...";
         }*/
        writeToAddress(next,a);
        pc+=3;
        cycles = 16;
    }
    else if (opcode == 0x28)//jr z,n
    {
        pc += 2;
        if (zeroStatus()) {
            pc += (int8_t) readFromAddress(address + 1);
            cycles = 12;
        } else{
            cycles = 8;
        }
    }
    else if (opcode == 0x20)//jr nz, n
    {
        pc += 2;//increment to next instruction
        if (!zeroStatus())
        {
            pc += (int8_t)readFromAddress(address+1);//jump
            cycles =12;
            // cout<<"Incrementing pc by " << to_string((int8_t)next)<<" bytes"<<endl;
        } else{ //if zero bit is set
            //cout<<"Zero not set, skipping jump"<<endl;
            cycles = 8;
        }
    }
    else if (opcode == 0x38)//jr c, n
    {
        pc+=2;
        if (carryStatus())
        {
            pc += (int8_t) readFromAddress(address + 1);
            cycles = 12;
        } else{
            cycles = 8;
        }

    }
    else if (opcode == 0x30)//jr nc, n
    {
        pc += 2;
        if (!carryStatus()) {
            pc += (int8_t) readFromAddress(address + 1);
            cycles = 12;
        } else{
            cycles = 8;
        }
    }

    else if (opcode == 0x2e)//ld l, n
    {
        l = readFromAddress(address+1);
        pc+=2;
        cycles = 8;
    }
    else if (opcode == 0x18)//jr n
    {
        pc+=2;
        pc += (int8_t) readFromAddress(address + 1);
        cycles = 12;
    }
    else if (opcode == 0x1e)//ld e, n
    {
        e = readFromAddress(address+1);
        pc+=2;
        cycles = 8;
    }
    else if (opcode == 0xc3)//jp nn
    {
        //cout<<"jumping from 0x"<<hex<<pc<<dec<<" to 0x"<<hex<<concat(readFromAddress(address+2),readFromAddress(address+1))<<dec<<endl;
        pc = concat(readFromAddress(address+2),readFromAddress(address+1));
        cycles = 16;
        if (pc == 0x0000)
        {
            //dumpWorkRamToFile("/home/andrew/Downloads/ram.bin");
            throw "Jumped to 0, probably a bug";
        }
    }
    else if (opcode == 0xca)//jp z, nn
    {
        if (zeroStatus())
        {
            pc = concat(readFromAddress(address+2),readFromAddress(address+1));
            cycles = 16;
        } else{
            pc+=3;
            cycles = 12;
        }
    }
    else if (opcode == 0xda)//jp c, nn
    {
        if (carryStatus())
        {
            pc = concat(readFromAddress(address+2),readFromAddress(address+1));
            cycles = 16;
        } else{
            pc+=3;
            cycles = 12;
        }
    }
    else if (opcode == 0xc2)//jp nz, nn
    {
        if (!zeroStatus())
        {
            pc = concat(readFromAddress(address+2),readFromAddress(address+1));
            cycles = 16;
        } else{
            pc+=3;
            cycles = 12;
        }
    }
    else if (opcode == 0xd2)//jp nc, nn
    {
        if (!carryStatus())
        {
            pc = concat(readFromAddress(address+2),readFromAddress(address+1));
            cycles = 16;
        } else{
            pc+=3;
            cycles = 12;
        }
    }
    else if (opcode == 0x47)//ld b,a
    {
        b=a;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x2a)//ld a, (hl+)
    {
        a = readFromAddress(concat(h,l));
        writePair(h,l,concat(h,l)+1);
        cycles = 8;
        pc++;
    }
    else if (opcode == 0x3a)//ld a, (hl-)
    {
        a = readFromAddress(concat(h,l));
        writePair(h,l,concat(h,l)-1);
        cycles = 8;
        pc++;
    }
    else if (opcode == 0xf0)//ld a, (0xff00+n)
    {
        a = readFromAddress(0xff00 + readFromAddress(address+1));
        pc+=2;
        cycles = 12;
    }
    else if (opcode == 0x12)//ld (de), a
    {
        writeToAddress(concat(d,e),a);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x78)//ld a, b
    {
        a=b;
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
        e=c;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x01)//ld bc, nn
    {
        b = readFromAddress(address+2);//read n into b
        c = readFromAddress(address+1);//read n into c
        pc+=3;
        cycles = 12;
    }
    else if (opcode == 0x0b)//dec bc
    {
        writePair(b,c, concat(b,c)-1);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x1b)//dec de
    {
        writePair(d,e,concat(d,e)-1);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0x2b)//dec hl
    {
        writePair(h,l,concat(d,e)-1);
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
        a=h;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x7d)//ld a,l
    {
        a=l;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0xf3)//di(disable interrupts)
    {
        cout<<"INFO: disabling interrupts.\n";
        enableInterrupts = false;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0xd9)//reti
    {
        enableInterrupts = true;
        pc = concat (readFromAddress(sp+1),readFromAddress(sp));
        sp += 2;
        cycles = 16;
    }
    else if (opcode == 0xfb)//ei (enable interrupts)
    {
        cout<<"INFO: Enabling interrupts.\n";
        enableInterrupts = true;
        pc++;
        cycles = 4;
    }
    else if (opcode == 0x26)//ld h, n
    {
        h = readFromAddress(address+1);
        cycles = 8;
        pc+=2;
    }
    else if (opcode == 0x0f)//rrca
    {
        uint8_t firstBit = ((a & 0x01));
        a = (a>>1);
        setCarry(false);
        if (firstBit == 0x01)
        {
            a |= 0x80;
            setCarry(true);
        }
        cycles = 4;
        pc++;
    }
    else if (opcode == 0x1f)//rra (fast rr a)
    {
        uint8_t firstBit = ((a & 0x01));
        a = (a>>1);
        if (carryStatus())
        {
            a = (a | 0x80);
        } else{
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
        if (carryStatus())
        {
            a++;
        }//if carry == 0, same as add a,n
        add8(a,readFromAddress(address+1));

        pc++;//because memory read
        cycles = 8;
    }
    else if (opcode == 0xe9)//jp hl
    {
        pc = concat(h,l);
        cycles = 4;
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
        int8_t offset = readFromAddress(address+1);
        writePair(h,l,sp+offset);
        cycles = 12;
        pc+=2;
        if (offset >= 0)
        {
            setCarry(((sp&0xff)+(offset))>0xff);
            setHalf(((sp&0xf)+(offset&0xf))>0xf);
        }
        else
        {
            setCarry(((sp+offset)&0xff)<=(sp&0xff));
            setHalf(((sp+offset)&0xf)<=(sp&0xf));
        }
        setSubtract(false);
        setZero(false);
        cout<<"\033[1;33mWARNING:\033[0m Carry and Half-carry flags with the \"ld hl, sp+i8\" instruction are not tested."<<endl;

    }
    else if (opcode == 0xf9)//ld sp, hl
    {
        sp = concat(h,l);
        cycles = 8;
        pc++;
    }
    else if (opcode == 0x27)//daa
    {
        if (!subtractStatus())
        {
            if (carryStatus() || a>0x99)
            {
                a+=0x60;
                setCarry(true);
            }
            if (halfStatus() || (a & 0x0f) > 0x09)
            {
                a+=0x06;
            }
        } else{
            if (carryStatus()){a-=0x60;}
            if (halfStatus()){a-=0x06;}
        }
        setZero(a==0);
        setHalf(false);
        cycles = 4;
        pc++;
    }
    else if (opcode == 0x07)//rlca
    {
        uint8_t firstBit = ((a & 0x80));
        a = (a<<1);
        a = a | (firstBit>>7);
        setCarry(firstBit == 0x80);
        setHalf(false);
        setSubtract(false);
        setZero(false);
        cycles=4;
        pc++;
    }
    else if (opcode == 0xc7)//rst 0x00
    {
        pc++;//?
        writeToAddress(sp-1, (uint8_t)(pc>>8));//write PCh to stack
        writeToAddress(sp-2, (uint8_t)(pc&0x00ff));//write PCl to stack
        sp-=2;
        cycles = 16;
        pc=0x0000;
    }
    else if (opcode == 0xd7)//rst 0x10
    {
        pc++;//?
        writeToAddress(sp-1, (uint8_t)(pc>>8));//write PCh to stack
        writeToAddress(sp-2, (uint8_t)(pc&0x00ff));//write PCl to stack
        sp-=2;
        cycles = 16;
        pc=0x0010;
    }
    else if (opcode == 0xe7)//rst 0x20
    {
        pc++;//?
        writeToAddress(sp-1, (uint8_t)(pc>>8));//write PCh to stack
        writeToAddress(sp-2, (uint8_t)(pc&0x00ff));//write PCl to stack
        sp-=2;
        cycles = 16;
        pc=0x0020;
    }
    else if (opcode == 0xf7)//rst 0x30
    {
        pc++;//?
        writeToAddress(sp-1, (uint8_t)(pc>>8));//write PCh to stack
        writeToAddress(sp-2, (uint8_t)(pc&0x00ff));//write PCl to stack
        sp-=2;
        cycles = 16;
        pc=0x0030;
    }
    else if (opcode == 0xcf)//rst 0x08
    {
        pc++;//?
        writeToAddress(sp-1, (uint8_t)(pc>>8));//write PCh to stack
        writeToAddress(sp-2, (uint8_t)(pc&0x00ff));//write PCl to stack
        //cout<<hex<<to_string((uint8_t)(pc>>8))<<dec<<endl;//h
        //cout<<hex<<to_string((uint8_t)(pc&0x00ff))<<dec<<endl;//l
        sp-=2;
        cycles = 16;
        pc=0x0008;
    }
    else if (opcode == 0xdf)//rst 0x18
    {
        pc++;//?
        writeToAddress(sp-1, (uint8_t)(pc>>8));//write PCh to stack
        writeToAddress(sp-2, (uint8_t)(pc&0x00ff));//write PCl to stack
        //cout<<hex<<to_string((uint8_t)(pc>>8))<<dec<<endl;//h
        //cout<<hex<<to_string((uint8_t)(pc&0x00ff))<<dec<<endl;//l
        sp-=2;
        cycles = 16;
        pc=0x0018;
    }
    else if (opcode == 0xef)//rst 0x28
    {
        pc++;//?
        writeToAddress(sp-1, (uint8_t)(pc>>8));//write PCh to stack
        writeToAddress(sp-2, (uint8_t)(pc&0x00ff));//write PCl to stack
        //cout<<hex<<to_string((uint8_t)(pc>>8))<<dec<<endl;//h
        //cout<<hex<<to_string((uint8_t)(pc&0x00ff))<<dec<<endl;//l
        sp-=2;
        cycles = 16;
        pc=0x0028;
    }
    else if (opcode == 0xff)//rst 0x38
    {
        cout<<"\033[1;33mWARNING:\033[0m RST 0x38 instruction encountered. Might be an error"<<endl;
        pc++;//?
        writeToAddress(sp-1, (uint8_t)(pc>>8));//write PCh to stack
        writeToAddress(sp-2, (uint8_t)(pc&0x00ff));//write PCl to stack
        //cout<<hex<<to_string((uint8_t)(pc>>8))<<dec<<endl;//h
        //cout<<hex<<to_string((uint8_t)(pc&0x00ff))<<dec<<endl;//l
        sp-=2;
        cycles = 16;
        pc=0x0038;
    }
    else if (opcode == 0x76)//halt
    {
        cycles = 4;
        pc++;
        halted = true;
        cout<<"WARNING: HALT bug is not implemented.\n";
    }
    else if (opcode == 0x3f)//ccf
    {
        setSubtract(false);
        setHalf(false);
        setCarry(!carryStatus());
        pc++;
        cycles=4;
    }
    else if (opcode == 0x02)//ld (bc), a
    {
        writeToAddress(concat(b,c), a);
        pc++;
        cycles = 8;
    }
    else if (opcode == 0xcb)//prefix for extended instructions
    {
        executePrefixedInstruction(readFromAddress(address+1));
    }
    else{
        errorAddress = -1;
        std::cout<<"At address 0x"<<hex<<address<<dec<<std::endl;
        cout<<"Opcode: 0x"<<hex<<(int)opcode<<dec <<std::endl;
        throw "OPCODE NOT IMPLEMENTED";
    }

    processTimer(cycles);
    checkInterrupts();

}


