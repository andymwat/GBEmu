//
// Created by andrew on 10/12/19.
//

#ifndef GBEMU_CARTRIDGE_H
#define GBEMU_CARTRIDGE_H


//#include <stdint-gcc.h>
#include <cstdint>

class cartridge {
public:
    uint8_t mbcType;
    uint8_t bankCount;
    uint8_t* banks;
    uint8_t* ramBanks;
    uint8_t ramBankIdentifier;
	uint16_t totalRamSize;
    cartridge(uint8_t, uint8_t, uint8_t);
    ~cartridge();
    static const uint16_t bankSize = 16384;
	static const uint16_t ramBankSize = 8192;
private:

};


#endif //GBEMU_CARTRIDGE_H
