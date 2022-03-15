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
// Created by andrew on 1/31/20.
//

#include <stdio.h>
#include <stdint.h>
#include "logger.h"



void logInfo(const char* str) {
    printf("\033[1;32m[INFO]: \033[0m%s\n", str);
}

void logError(const char* str, uint16_t address, uint8_t data) {

    printf("\033[1;31m[ERROR]: \033[0m%s\n", str);
    printf("Address: 0x%X, data: 0x%X", address, data);

}

void logWarning(const char* str, uint16_t address, uint8_t data) {


    printf("\033[1;33m[WARNING]: \033[0m%s\n", str);
    printf("Address: 0x%X, data: 0x%X", address, data);

}

void logErrorNoData(const char* str) {
    printf("\033[1;31m[ERROR]: \033[0m%s\n", str);
}

void logWarningNoData(const char* str)
{
    printf("\033[1;33m[WARNING]: \033[0m%s\n", str);
}

