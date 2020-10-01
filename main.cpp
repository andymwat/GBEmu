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


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"


#include <iostream>
#include <fstream>
#include <vector>
#include <SDL.h>
//Define as true to run tests
#define RUN_DEBUG_TESTS false




//Gotta do this for SDL for some reason
#undef main
//
#include "cartridge.h"
#include "main.h"
#include "interpreter.h"
#include "instructionDecoder.h"
#include "lcdController.h"
#include "keyboardInput.h"
#include "logger.h"

#include "audioController.h"
#include "tests.h"


#ifdef PLATFORM_UNIX
#include <unistd.h>
#endif

using namespace std;
SDL_Event events;





int main(int argc, char* args[])
{
	cout << "GBEmu  Copyright (C) 2020 Andrew Watson\nThis program comes with ABSOLUTELY NO WARRANTY; for details, see the included LICENSE file or visit https://www.gnu.org/licenses/ \n";
    logger::logInfo("Initializing window...");
    initWindow();
	logger::logInfo("Initializing audio...");
    if (initAudio() != 0)
    {
		logger::logErrorNoData("Could not initialize audio!");
        return -1;
    }
    lcdEnable = true;
    a=b=c=d=e=f=h=l=sp=pc=0;
	logger::logInfo("Loading ROM...");
	if (argc == 1)
	{
		loadTestRom("C:/Users/andym/Downloads/ROMs/GBEmu/kirby.gb");
	}
	else if (argc == 2)
	{
		loadTestRom(args[1]);
	}
	else
	{
		cout << "Provide a ROM to run as an argument, or run GBEmu with no arguments to run the test ROM.\n";
	}




    if (RUN_DEBUG_TESTS)
    {
            std::vector<uint8_t> instructions = { 0x00, 0x00, 0x04,0x05,0x14,0x15,0x24,0x25 };
            cpuRegisterState startState, endState;
            startState.a = startState.b = startState.c = startState.d = startState.e = startState.f = startState.h = startState.l = 0x00;
            startState.pc = 0x0000;
            startState.sp = 0x0000;

            endState = startState;
            endState.f = 0xc0;

            if (runTest(startState, instructions, endState) == 0)
            {
                    cout << "All tests executed successfully.\n";
            }

#if defined(_WIN32)
            system("pause");
#endif
            return 0;

    }

    std::cout << "Loading from file: " << filePath << ".sav\n";
    std::string savePath = filePath + ".sav";
    FILE* file = fopen(savePath.c_str(), "rb");
    if (file == nullptr)
    {
        std::cout << "Failed to open save file! This is normal the first time loading a ROM, if a file wasn't saved previously, or if the ROM has no battery-backed RAM.\n";
    }
    else
    {
        fread(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, file);
        fclose(file);
        std::cout << "Loaded!\n";
    }

    try {
        m_TimerCounter = 1024;
        initRegisters();//ignore bootrom
        cout<<"ROM loaded. Starting emulation..."<<endl;
        //usleep(50000);
        while(true)
        {
			keyboardBreak = false;
            dumpRegisters();
            int sel;
            cout<<"pc to run until, or 0 for step: ";
            cin.unsetf(ios::dec);
            cin.unsetf(ios::hex);
            cin>>sel;
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
                if (sp == 0)
                {
                    //logger::logWarning("Stack pointer is zero.", pc, readFromAddress(pc));
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
                        if (sp == 0)
                        {
                                //logger::logWarning("Stack pointer is zero.", pc, readFromAddress(pc));
                        }
                        updateScreen(cycles);
                        updateAudio(cycles);
                        cyclesUntilUpdate -= cycles;
                    }




                    if (saveToFile && !saved)
                    {
                        saveToFile = false;
                        std::cout << "Saving to file: " << filePath << ".sav\n";
                        std::string savePath = filePath + ".sav";
                        FILE* file = fopen(savePath.c_str(), "wb");
                        fwrite(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, file);
                        fclose(file);
                        std::cout << "Saved!\n";
                        saved = true;
                    }
                    if (loadFromFile && !loaded)
                    {
                        loadFromFile = false;
                        std::cout << "Loading from file: " << filePath << ".sav\n";
                        std::string savePath = filePath + ".sav";
                        FILE* file = fopen(savePath.c_str(), "rb");
                        if (file == nullptr)
                        {
                            std::cout << "Failed to open file!\n";
                        }
                        else
                        {
                            fread(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, file);
                            fclose(file);
                            std::cout << "Loaded!\n";
                        }
                        loaded = true;
                    }
                }

                if (keyboardBreak)
                {
                        cout << "Got keyboard breakpoint.\n";
                }


            }

        }
    }
    catch (const char* msg)
    {
        cout<<msg<<endl;
        if (errorAddress >= 0)
        {
            logger::logError("Error accessing address, data could not be read (disregard data).", errorAddress, 0);
        }
        dumpRegisters();
    }
    cout<<output<<endl;
	SDL_PauseAudio(1);
#if defined(_WIN32)
	system("pause");
#endif
   
    SDL_DestroyWindow(window);
    SDL_Quit();
	
    return 0;
}

#pragma clang diagnostic pop
