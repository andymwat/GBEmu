//
// Created by andrew on 10/12/19.
//

#include "cartridge.h"
#include "main.h"
#include <cmath>
#include <iostream>

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
            std::cout<<"INFO: Loaded cart with no cartRAM.\n";
            break;
        case 1:
            std::cout<<"INFO: Loaded cart with 2048 bytes of cartRAM.\n";
            this->ramBanks = new uint8_t[2048];
            break;
        case 2:
            std::cout<<"INFO: Loaded cart with 8192 bytes of cartRAM.\n";
            this->ramBanks = new uint8_t[8192];
            break;
        case 3:
            std::cout<<"INFO: Loaded cart with 32768 bytes (4*8192 bytes) of cartRAM.\n";
            this->ramBanks = new uint8_t[32768];
            break;
        default:
            std::cout<<"ramBankNum is 0x"<<std::hex<<(uint16_t)ramBankNum<<std::dec<<std::endl;
            throw "Invalid ramBank identifier!";

    }

    if (banks == nullptr)
    {
        std::cout<<"ERROR: Could not allocate memory for ROM."<<std::endl;
    }
    std::cout<<"CARTRIDGE INFO: Allocated " <<to_string(bankSize*numberOfBanks)<<" bytes for cartridge ROM."<<std::endl;
    std::cout<<"Bank size: "<<to_string(bankSize)<<std::endl;
}
cartridge::~cartridge() {
    std::cout<<"Freeing ROM and RAM memory..."<<std::endl;
   // usleep(5000000);
    delete banks;
    delete ramBanks;
}
