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
// Created by andrew on 11/19/19.
//

#include "keyboardInput.h"
#include "interpreter.h"
#include "lcdController.h"
#include "logger.h"

uint8_t joypadStateInternal;
uint8_t previousJoypadState;
bool keyboardBreak = false;


bool saveToFile = false;
bool saved = false;
bool loadFromFile = false;
bool loaded= false;
bool fastForward = false;


/*Bits:
 * NOTE: 0=selected/pressed
 * 7: not used
 * 6: not used
 * 5: Button select
 * 4: Direction select
 * 3: Down or start
 * 2: Up or select
 * 1: Left or B
 * 0: Right or A
 */
bool b_up, b_down, b_left, b_right, b_a, b_b, b_start, b_select, b_increaseScreenSize, b_decreaseScreenSize;

bool screenSizePressedLastFrame = false;
void checkKeyboardNew()
{
    //TODO: check input
    logErrorNoData("Unimplemented input check");
    exit(-1);
    /*
	uint8_t *keyboardState = (uint8_t*)SDL_GetKeyboardState(NULL); //get keyboard state
	b_up = keyboardState[SDL_SCANCODE_UP];
	b_down = keyboardState[SDL_SCANCODE_DOWN];
	b_left = keyboardState[SDL_SCANCODE_LEFT];
	b_right = keyboardState[SDL_SCANCODE_RIGHT];

	b_a = keyboardState[SDL_SCANCODE_Z];
	b_b = keyboardState[SDL_SCANCODE_X];
	b_start = keyboardState[SDL_SCANCODE_A];
	b_select = keyboardState[SDL_SCANCODE_S];
	
	keyboardBreak = keyboardState[SDL_SCANCODE_B];
	fastForward = keyboardState[SDL_SCANCODE_SPACE];
	saveToFile = keyboardState[SDL_SCANCODE_P];
	b_increaseScreenSize = keyboardState[SDL_SCANCODE_RIGHTBRACKET];
	b_decreaseScreenSize = keyboardState[SDL_SCANCODE_LEFTBRACKET];
    */

    //TODO: load and save from file
    logErrorNoData("Unimplemented load and save to file.");
    exit(-1);
    /*
	if (!keyboardState[SDL_SCANCODE_P])
	{
		saved = false;
	}
	loadFromFile = keyboardState[SDL_SCANCODE_L];
	if (!keyboardState[SDL_SCANCODE_L])
	{
		loaded = false;
	}
    */


	joypadRegister |= 0xf;//set bottom for bits, to be cleared below
	if (!TestBit(joypadRegister, 5))//button select
	{
		if (b_a)
		{
			joypadRegister = BitReset(joypadRegister, 0);
		}
		if (b_b)
		{
			joypadRegister = BitReset(joypadRegister, 1);
		}
		if (b_select)
		{
			joypadRegister = BitReset(joypadRegister, 2);
		}
		if (b_start)
		{
			joypadRegister = BitReset(joypadRegister, 3);
		}
	}
	if (!TestBit(joypadRegister, 4))//direction select
	{
		if (b_right)
		{
			joypadRegister = BitReset(joypadRegister, 0);
		}
		if (b_left)
		{
			joypadRegister = BitReset(joypadRegister, 1);
		}
		if (b_up)
		{
			joypadRegister = BitReset(joypadRegister, 2);
		}
		if (b_down)
		{
			joypadRegister = BitReset(joypadRegister, 3);
		}
	}
	if ((TestBit(joypadRegister, 5))&& (TestBit(joypadRegister, 4)))
	{
		//logWarningNoData("No joypad selection set!");
	}

	//test to see if button was pressed just this frame
	if (TestBit(previousJoypadState,0) && !TestBit(joypadRegister,0))
		interruptFlag |= 0x10;
	if (TestBit(previousJoypadState, 1) && !TestBit(joypadRegister, 1))
		interruptFlag |= 0x10;
	if (TestBit(previousJoypadState, 2) && !TestBit(joypadRegister, 2))
		interruptFlag |= 0x10;
	if (TestBit(previousJoypadState, 3) && !TestBit(joypadRegister, 3))
		interruptFlag |= 0x10;

	if (TestBit(interruptFlag, 4))
	{
		//logInfo("Requesting joypad interrupt.");
	}
	previousJoypadState = joypadRegister;
}


uint8_t getJoypadState() {
    joypadRegister ^= 0xff;
    if (!TestBit(joypadRegister, 4))
    {
        uint8_t topJoypad = joypadStateInternal >> 4;
        topJoypad |= 0xf0;
        joypadRegister &= topJoypad;
    }
    else if (!TestBit(joypadRegister,5))
    {
        uint8_t bottomJoypad = joypadStateInternal & 0xf;
        bottomJoypad |= 0xf0;
        joypadRegister &= bottomJoypad;
    }
    return joypadRegister;
}

void KeyPressed(int key) {
    bool previouslyUnset = false;
    if (!TestBit(joypadStateInternal, key))
        previouslyUnset = true;
    joypadStateInternal = BitReset(joypadStateInternal, key);
    bool button = key > 3;
    bool needsInterrupt = false;
    if (button && !TestBit(joypadRegister, 5))
        needsInterrupt = true;
    else if (!button && !TestBit(joypadRegister, 4))
        needsInterrupt = true;
    if (needsInterrupt && !previouslyUnset)
        interruptFlag |= 0x10;

}
void KeyReleased(int key)
{
    joypadStateInternal = BitSet(joypadStateInternal, key);
}
