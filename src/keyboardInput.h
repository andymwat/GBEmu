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
// Created by andrew on 11/19/19.
//


#ifndef GBEMU_KEYBOARDINPUT_H
#define GBEMU_KEYBOARDINPUT_H

#include <cstdint>
#include <SDL.h>
void checkKeyboard(SDL_Event events);
void checkKeyboardNew();
extern uint8_t keys;
//void
uint8_t getJoypadState();
void KeyPressed(int key);
void KeyReleased(int key);
extern uint8_t joypadStateInternal;
extern uint8_t previousJoypadState;
extern bool keyboardBreak;
extern bool saveToFile, saved, loadFromFile, loaded, fastForward;

#endif //GBEMU_KEYBOARDINPUT_H
