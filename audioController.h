
#ifndef GBEMU_AUDIOCONTROLLER_H
#define GBEMU_AUDIOCONTROLLER_H
#include <iostream>
#include <fstream>
#include <vector>
#include <SDL.h>
#include "logger.h"

int initAudio(void);

void writeToAudioRegister(uint16_t address, uint8_t data);
uint8_t readFromAudioRegister(uint16_t address);
void updateAudio(uint8_t cycles);

#endif //GBEMU_AUDIOCONTROLLER_H