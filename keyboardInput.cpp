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
/*Bits:
 * 7: not used
 * 6: not used
 * 5: Button select
 * 4: Direction select
 * 3: Down or start
 * 2: Up or select
 * 1: Left or B
 * 0: Right or A
 */

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
