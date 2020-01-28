//
// Created by andrew on 11/19/19.
//


#ifndef GBEMU_KEYBOARDINPUT_H
#define GBEMU_KEYBOARDINPUT_H

#include <cstdint>
#include <SDL.h>
void checkKeyboard(SDL_Event events);
extern uint8_t keys;
//void
uint8_t getJoypadState();
void KeyPressed(int key);
void KeyReleased(int key);
extern uint8_t joypadStateInternal;

#endif //GBEMU_KEYBOARDINPUT_H
