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
//Define as true to run tests
#define RUN_DEBUG_TESTS false

#include "cartridge.h"
#include "main.h"
#include "interpreter.h"
#include "instructionDecoder.h"
#include "lcdController.h"
#include "keyboardInput.h"
#include "logger.h"

#include "audioController.h"


const char* testRomPath = "C:/Users/andym/Downloads/ROMs/GBEmu/kirby.gb";

int main(int argc, char* args[])
{
	printf("GBEmu  Copyright (C) 2020 Andrew Watson\nThis program comes with ABSOLUTELY NO WARRANTY; for details, see the included LICENSE file or visit https://www.gnu.org/licenses/ \n");
    logInfo("Initializing window...");
    initWindow();
	logInfo("Initializing audio...");
    if (initAudio() != 0)
    {
		logErrorNoData("Could not initialize audio!");
		printf("Continue [y/n]? ");
		char input = getchar();
		if (input != 'Y' && input != 'y')
		{
			return -1;
		}
    }
    lcdEnable = true;
    a=b=c=d=e=f=h=l=sp=pc=0;
	logInfo("Loading ROM...");
	if (argc == 1)
	{
		loadTestRom(testRomPath);
	}
	else if (argc == 2)
	{
		loadTestRom(args[1]);
	}
	else
	{
		printf("Provide a ROM to run as an argument, or run GBEmu with no arguments to run the test ROM.\n");
	}




    char savePath[256];
    snprintf(savePath, sizeof(savePath), "%s%s", filePath, ".sav");
    FILE* file = fopen(savePath, "rb");
    if (file == NULL)
    {
        logWarningNoData( "Failed to open save file! This is normal the first time loading a ROM, if a file wasn't saved previously, or if the ROM has no battery-backed RAM.");
    }
    else
    {
        int result = fread(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, file);
        if (result != currentCartridge->totalRamSize)
        {
            logErrorNoData("Error reading from save file!");
        }
        fclose(file);
    }

        m_TimerCounter = 1024;
        initRegisters();//ignore bootrom
        logInfo("ROM loaded. Starting emulation...");
        //usleep(50000);

    while(true)
    {
        keyboardBreak = false;
        while (!keyboardBreak)
        {
            unsigned int cyclesUntilUpdate = updateFrequency;
            while (cyclesUntilUpdate >= 0 && !keyboardBreak)
            {
                keyboardBreak = false;

                checkKeyboardNew();
                cycles = 0;

                execute(pc);

                if (cycles == 0)
                {
                    logWarning("Cycle count not set.", pc, readFromAddress(pc));
                }
                updateScreen(cycles);
                updateAudio(cycles);
                cyclesUntilUpdate -= cycles;
            }

            //Save SaveRAM to file
            if (saveToFile && !saved)
            {
                saveToFile = false;
                printf("Saving to file: %s.sav\n", filePath);
                FILE* file = fopen(savePath, "wb");
                fwrite(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, file);
                fclose(file);
                printf("Saved!\n");
                saved = true;
            }
            //Load SaveRAM from file
            if (loadFromFile && !loaded)
            {
                loadFromFile = false;
                printf("Loading from file: %s.sav\n", filePath);
                FILE* file = fopen(savePath, "rb");
                if (file == NULL)
                {
                    printf("Failed to open file!\n");
                }
                else
                {
                    fread(currentCartridge->ramBanks, sizeof(uint8_t), currentCartridge->totalRamSize, file);
                    fclose(file);
                    printf("Loaded!\n");
                }
                loaded = true;
            }
        }
        dumpRegisters();

        if (keyboardBreak)
        {
            printf("Got keyboard breakpoint.\n");
        }

    }
}

