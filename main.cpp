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

#include "tests.h"

using namespace std;
SDL_Event events;


int main(int argc, char* args[])
{
    cout<<"Initializing window..."<<endl;
    initWindow();
    lcdEnable = true;
    a=b=c=d=e=f=h=l=sp=pc=0;
    cout<<"Loading ROM..."<<endl;
   //loadTestRom("/home/andrew/Downloads/GBemu/cpu_instrs/cpu_instrs.gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/tetris.gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/sml.gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/cpu_instrs/individual/11-op a,(hl).gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/drMario.gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/kirby.gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/loz.gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/pkmn.gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/sml2.gb");
    //loadTestRom("/home/andrew/Downloads/GBemu/new/dkl.gb");
    //loadTestRom("/home/andrew/Downloads/DMG_ROM.bin");

	//loadTestRom("C:/Users/andym/Downloads/ROMs/gb-test-roms-master/cpu_instrs/cpu_instrs.gb");
	loadTestRom("C:/Users/andym/Downloads/ROMs/gb-test-roms-master/cpu_instrs/individual/09-op r,r.gb");
	//loadTestRom("C:/Users/andym/Downloads/ROMs/GBEmu/sml.gb");
	//loadTestRom("C:/Users/andym/Downloads/ROMs/GBEmu/tetris.gb");


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



    try {
        pc = 0x100;
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
					keyboardBreak = false;

					uint16_t oldPc = pc;
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

					if (oldPc == pc)
					{
						logger::logWarningNoData("PC did not change from previous instruction. Might be an error.");
					}

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

#if defined(_WIN32)
	system("pause");
#endif
   
    SDL_DestroyWindow(window);
    SDL_Quit();
	
    return 0;
}

#pragma clang diagnostic pop
