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
#ifndef GBEMU_AUDIOCONTROLLER_H
#define GBEMU_AUDIOCONTROLLER_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "logger.h"

int initAudio(void);

void writeToAudioRegister(uint16_t address, uint8_t data);
uint8_t readFromAudioRegister(uint16_t address);
void updateAudio(uint8_t cycles);
void mixAudio();

#endif //GBEMU_AUDIOCONTROLLER_H