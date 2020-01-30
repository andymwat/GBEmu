
#ifndef GBEMU_INTSTRUCTIONDECODER_H
#define GBEMU_INTSTRUCTIONDECODER_H
#include <cstdint>

void execute(uint16_t);

bool executeArithmetic(uint8_t, uint16_t);
bool executeStructural(uint8_t, uint16_t);

#endif //GBEMU_INSTRUCTIONDECODER_H