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
//
// Created by andrew on 10/12/19.
//

#include "cartridge.h"
#include "main.h"
#include <cmath>
#include <iostream>
#include <memory>
#include "logger.h"

//#include <unistd.h>

cartridge::cartridge(uint8_t mbcNumber, uint8_t bankNum, uint8_t ramBankNum) {
    this->mbcType = mbcNumber;
    this->bankCount = bankNum;
    this->ramBankIdentifier = ramBankNum;
    this->banks = nullptr;
	
    uint8_t numberOfBanks;
    if (bankNum <= 0x07)//2^(n+1) banks
    {
	    numberOfBanks = pow(2,(bankNum+1));
    }
    else
    {
	    throw "UNIMPLEMENTED BANK COUNT";
    }
    this->banks = new uint8_t[bankSize*numberOfBanks];

    switch (ramBankNum)
    {
        case 0:
            logger::logInfo("Loaded cart with no cartRAM.");
            break;
        case 1:
            logger::logInfo("Loaded cart with 2048 bytes of RAM");
            this->ramBanks = new uint8_t[2048];
			this->totalRamSize = 2048;
            break;
        case 2:
            logger::logInfo("Loaded cart with 8192 bytes of RAM");
            this->ramBanks = new uint8_t[8192];
			this->totalRamSize = 8192;
            break;
        case 3:
            logger::logInfo("Loaded cart with 32768 bytes of RAM");
            this->ramBanks = new uint8_t[32768];
			this->totalRamSize = 32768;
            break;
        default:
            logger::logErrorNoData("ramBankNum is " + to_string(ramBankNum));
            throw "Invalid ramBank identifier!";

    }

    if (banks == nullptr)
    {
        logger::logErrorNoData("Could not allocate memory for ROM.");
    }
    logger::logInfo("Allocated " + to_string(bankSize*numberOfBanks) + " bytes for cartridge ROM.");
}
cartridge::~cartridge() {
    logger::logInfo("Freeing ROM and RAM memory...");
    delete banks;
    delete ramBanks;
}
