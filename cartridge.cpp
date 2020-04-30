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

#include "cartridge.h"
#include "main.h"
#include <cmath>
#include <iostream>
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
            //std::cout<<"INFO: Loaded cart with no cartRAM.\n";
            break;
        case 1:
            logger::logInfo("Loaded cart with 2048 bytes of RAM");
            //std::cout<<"INFO: Loaded cart with 2048 bytes of cartRAM.\n";
            this->ramBanks = new uint8_t[2048];
			this->totalRamSize = 2048;
            break;
        case 2:
            logger::logInfo("Loaded cart with 8192 bytes of RAM");
            //std::cout<<"INFO: Loaded cart with 8192 bytes of cartRAM.\n";
            this->ramBanks = new uint8_t[8192];
			this->totalRamSize = 8192;
            break;
        case 3:
            logger::logInfo("Loaded cart with 32768 bytes of RAM");
           // std::cout<<"INFO: Loaded cart with 32768 bytes (4*8192 bytes) of cartRAM.\n";
            this->ramBanks = new uint8_t[32768];
			this->totalRamSize = 32768;
            break;
        default:
            logger::logErrorNoData("ramBankNum is " + to_string(ramBankNum));
            //std::cout<<"ramBankNum is 0x"<<std::hex<<(uint16_t)ramBankNum<<std::dec<<std::endl;
            throw "Invalid ramBank identifier!";

    }

    if (banks == nullptr)
    {
        logger::logErrorNoData("Could not allocate memory for ROM.");
        //std::cout<<"ERROR: Could not allocate memory for ROM."<<std::endl;
    }
    logger::logInfo("Allocated " + to_string(bankSize*numberOfBanks) + " bytes for cartridge ROM.");
    //std::cout<<"CARTRIDGE INFO: Allocated " <<to_string(bankSize*numberOfBanks)<<" bytes for cartridge ROM."<<std::endl;
    //std::cout<<"Bank size: "<<to_string(bankSize)<<std::endl;
}
cartridge::~cartridge() {
    std::cout<<"Freeing ROM and RAM memory..."<<std::endl;
   // usleep(5000000);
    delete banks;
    delete ramBanks;
}
