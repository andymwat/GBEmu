//
// Created by andrew on 11/19/19.
//

#include "keyboardInput.h"
#include "interpreter.h"
#include "lcdController.h"
#include <SDL.h>
#include <iostream>
#include "logger.h"

uint8_t joypadStateInternal;
uint8_t previousJoypadState;
bool keyboardBreak = false;
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
bool b_up, b_down, b_left, b_right, b_a, b_b, b_start, b_select;
void checkKeyboardNew()
{
	//SDL_PumpEvents();

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
		//logger::logWarningNoData("No joypad selection set!");
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
		//logger::logInfo("Requesting joypad interrupt.");
	}
	//std::cout << "JOYPAD: 0x" << std::hex<<(uint16_t)joypadRegister<<std::dec << std::endl;
	previousJoypadState = joypadRegister;
}


void checkKeyboard(SDL_Event events) 
{
    if (events.type == SDL_KEYDOWN)
    {
		logger::logInfo("[JOYPAD]: Got key down.");
        switch (events.key.keysym.sym)
        {
            case SDLK_UP:
                KeyPressed(2);
                break;
            case SDLK_DOWN:
                KeyPressed(3);
                break;
            case SDLK_LEFT:
                KeyPressed(1);
                break;
            case SDLK_RIGHT:
                KeyPressed(2);
                break;

            case SDLK_a:
                KeyPressed(4);
                break;
            case SDLK_s:
                KeyPressed(5);
                break;
            case SDLK_RETURN:
                KeyPressed(7);
                break;
            case SDLK_SPACE:
                KeyPressed(6);
                break;
        }
    }
    if (events.type == SDL_KEYUP)
    {
		logger::logInfo("[JOYPAD]: Got key up.");
        switch (events.key.keysym.sym)
        {
            case SDLK_UP:
                KeyReleased(2);
                break;
            case SDLK_DOWN:
                KeyReleased(3);
                break;
            case SDLK_LEFT:
                KeyReleased(1);
                break;
            case SDLK_RIGHT:
                KeyReleased(2);
                break;

            case SDLK_a:
                KeyReleased(4);
                break;
            case SDLK_s:
                KeyReleased(5);
                break;
            case SDLK_RETURN:
                KeyReleased(7);
                break;
            case SDLK_SPACE:
                KeyReleased(6);
                break;
        }
    }
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
