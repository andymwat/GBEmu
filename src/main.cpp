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


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

//Long-term TODO: remove all global variables

#include <iostream>
#include <fstream>
#include <vector>
#include <SDL.h>
//Define as true to run tests
#define RUN_DEBUG_TESTS false
//Define as true to show single-stepping prompt before running ROM
#define STEPPING false



//Need do this for SDL for some reason
#undef main


#include "cartridge.h"
#include "main.h"
#include "interpreter.h"
#include "instructionDecoder.h"
#include "lcdController.h"
#include "keyboardInput.h"
#include "logger.h"

#include "audioController.h"
#include "tests.h"
#include "exceptions.h"


#ifdef PLATFORM_UNIX
#include <unistd.h>
#endif

using namespace std;
SDL_Event events;



int main(int argc, char* args[])
{
	printf("GBEmu  Copyright (C) 2022 Andrew Watson\nThis program comes with ABSOLUTELY NO WARRANTY; for details, see the included LICENSE file or visit https://www.gnu.org/licenses/ \n");
    
#ifdef PLATFORM_UNIX
	string testROMPath = "/home/andrew/Downloads/GBemu/sml.gb";
#else
	string testROMPath = "C:/Users/andym/Downloads/ROMs/GBEmu/tetris.gb";
#endif

	logger::logInfo("Initializing window...");
    initWindow();
	logger::logInfo("Initializing audio...");
    if (initAudio() != 0)
    {
		logger::logErrorNoData("Could not initialize audio!");
		std::printf("Continue [y/n]? ");
		int input = getchar();
		if (input != 'Y' && input != 'y')
		{
			return -1;
		}
    }
    lcdEnable = true;
    a=b=c=d=e=f=h=l=sp=pc=0;
	logger::logInfo("Loading ROM...");
	if (argc == 1)
	{
		loadTestRom(testROMPath);
	}
	else if (argc == 2)
	{
		loadTestRom(args[1]);
	}
	else
	{
		logger::logErrorNoData("Provide a ROM to run as an argument, or run GBEmu with no arguments to run the test ROM.\n");
	}

	
    std::string str = "Loading from saveFile: " + filePath + ".sav";
    logger::logInfo(str);
    std::string savePath = filePath + ".sav";
    FILE* saveFile = fopen(savePath.c_str(), "rb");
    if (saveFile == nullptr)
    {
        logger::logWarningNoData( "Failed to open save saveFile! This is normal the first time loading a ROM, if a saveFile wasn't saved previously, or if the ROM has no battery-backed RAM.");
    }
    else
    {
        int result = fread(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, saveFile);
        if (result != currentCartridge->totalRamSize)
        {
            logger::logErrorNoData("Error reading from save saveFile! Number of bytes read is ot the same as the cartridge's RAM size!");
        }
        fclose(saveFile);
    }

    try {
        m_TimerCounter = 1024;
        initRegisters();//ignore bootrom
        logger::logInfo("ROM loaded. Starting emulation...");



        //main loop
        while(true)
        {
            keyboardBreak = false;
            int sel = -1;
            if (STEPPING) {
                dumpRegisters();
                cout<<"pc to run until, or 0 for step: ";
                cin.unsetf(ios::dec);
                cin.unsetf(ios::hex);
                cin>>sel;
            }

            if (sel==0)
            {
                while (SDL_PollEvent(&events) != 0)
                {
                    if (events.type == SDL_QUIT)
                    {
                        throw "User exit.";
                    }
                }
                checkKeyboardNew();
                cycles = 0;
                execute(pc);
                if (cycles == 0)
                {
                    logger::logWarning("Cycle count not set.", pc, readFromAddress(pc));
                }
                updateScreen(cycles);
            }
            else{
				
                while (pc != sel && !keyboardBreak)
                {
                    int cyclesUntilUpdate = updateFrequency;
                    while (SDL_PollEvent(&events) != 0)//check for sdl events
                    {
                        if (events.type == SDL_QUIT)
                        {
                            throw "User exit.";
                        }
                    }
                    while (cyclesUntilUpdate >= 0 && !keyboardBreak)
                    {
                        keyboardBreak = false;

                        checkKeyboardNew();
                        cycles = 0;


                        execute(pc);

                        if (cycles == 0)
                        {
                            logger::logWarning("Cycle count not set.", pc, readFromAddress(pc));
                        }
                        updateScreen(cycles);
                        updateAudio(cycles);
                        cyclesUntilUpdate -= cycles;
                    }

					//Saving saveRAM to saveFile
                    if (saveToFile && !saved)
                    {
                        saveToFile = false;
						savePath = filePath + ".sav";
                        logger::logInfo("Saving to saveFile: " + savePath + "\n");
                        FILE* file = fopen(savePath.c_str(), "wb");
						if (file == nullptr)
						{
							logger::logErrorNoData("Failed to open saveFile!\n");
						}
						else 
						{
							fwrite(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, file);
							fclose(file);
							logger::logInfo("Saved!\n");
						}
                        
                        saved = true;
                    }

					//Loading saveRAM from saveFile
                    if (loadFromFile && !loaded)
                    {
                        loadFromFile = false;
						savePath = filePath + ".sav";
                        logger::logInfo("Loading from saveFile: " + savePath + "\n");
                        FILE* file = fopen(savePath.c_str(), "rb");
                        if (file == nullptr)
                        {
                            logger::logErrorNoData("Failed to open saveFile!\n");
                        }
                        else
                        {
                            fread(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, file);
                            fclose(file);
                            logger::logInfo("Loaded!\n");
                        }
                        loaded = true;
                    }
                }

                if (keyboardBreak)
                {
					logger::logInfo("Got keyboard breakpoint.\n");
                }

            }

        }
    }
    catch (const char* msg)
    {
		logger::logErrorNoData(msg);
        if (errorAddress >= 0)
        {
            logger::logError("Error accessing address, data could not be read (disregard data).", (uint16_t)errorAddress, 0);
        }
        dumpRegisters();
    }

	SDL_PauseAudio(1);

#if defined(_WIN32)
	system("pause");
#endif
   
    SDL_DestroyWindow(window);
    SDL_Quit();
	
    return 0;
}

#pragma clang diagnostic pop
