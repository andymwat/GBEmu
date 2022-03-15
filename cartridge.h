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
// Created by andrew on 10/12/19.
//

#ifndef GBEMU_CARTRIDGE_H
#define GBEMU_CARTRIDGE_H

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

typedef struct {
    uint8_t mbcType;
    uint8_t bankCount;
    uint8_t* banks;
    uint8_t* ramBanks;
    uint8_t ramBankIdentifier;
	uint16_t totalRamSize;

} cartridge;


const uint16_t bankSize = 16384;
const uint16_t ramBankSize = 8192;


cartridge* newCartridge(uint8_t, uint8_t, uint8_t);
void destroyCartridge(cartridge*);

#endif //GBEMU_CARTRIDGE_H
