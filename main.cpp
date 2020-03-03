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
	loadTestRom("C:/Users/andym/Downloads/ROMs/GBEmu/sml.gb");
	//loadTestRom("C:/Users/andym/Downloads/ROMs/GBEmu/tetris2.gb");


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
                while (pc != sel)
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
    /*}
    else if (selection == 2)
    {
        int prog;
        cout<<"Run until (decimal): ";
        cin>>prog;
        try {
            pc = 0x100;
            initRegisters();//ignore bootrom
            cout<<"ROM loaded. Starting emulation in 5 seconds..."<<endl;
            usleep(5000000);
            while(pc!=prog)
            {
                SDL_PollEvent(&events);
                if (events.type == SDL_QUIT)
                {
                    throw "User exit.";
                }
                cycles = 0;
                execute(pc);
                if (cycles == 0)
                {
                    cout<<"\033[1;33mWARNING:\033[0m Cycle count not set."<<endl;
                }
                if (sp == 0)
                {
                    cout<<"\033[1;33mWARNING:\033[0m Stack pointer is 0."<<endl;

                }
                updateScreen(cycles);
                //usleep(cycles*cycleTime);
            }
        }
        catch (const char* msg)
        {
            cout<<msg<<endl;
            if (errorAddress >= 0)
            {
                cout<<"Error accessing address "<<errorAddress<<"\t0x"<<hex<<errorAddress<<endl;
            }
            dumpRegisters();
        }
        cout<<output;

    }
    else if (selection == 3)//debug
    {
        pc = 0x100;
        initRegisters();//ignore bootrom
        cout<<"ROM loaded. Starting emulation in 5 seconds..."<<endl;
        usleep(5000000);

        cout<<endl<<"Debug mode enabled."<<endl;
        bool running = true;
        while (running)
        {
            SDL_PollEvent(&events);
            if (events.type == SDL_QUIT)
            {
                throw "User exit.";
            }
            dumpRegisters();
            cout<<"Options: "<<endl;
            cout<<"1: Step forward one instruction"<<endl;
            cout<<"2: Run normally."<<endl;
            cout<<"3: Run next n instructions."<<endl;
            cout<<"4: Run normally until a warning."<<endl;
            cout<<"5: Run until a memory address is accessed."<<endl;
            cout<<"6: Exit"<<endl;
            cout<<endl<<"Selection: ";
            cin>>selection;
            if (selection == 1)//step
            {
                try {
                    cycles = 0;
                    execute(pc);
                    if (cycles == 0)
                    {
                        cout<<"\033[1;33mWARNING:\033[0m Cycle count not set."<<endl;
                    }
                    if (sp == 0)
                    {
                        cout<<"\033[1;33mWARNING:\033[0m Stack pointer is 0."<<endl;
                    }
                    updateScreen(cycles);
                    //usleep(cycles*cycleTime);

                }
                catch (const char* msg)
                {
                    cout<<msg<<endl;
                    if (errorAddress >= 0)
                    {
                        cout<<"Error accessing address "<<errorAddress<<"\t0x"<<hex<<errorAddress<<endl;
                    }
                    dumpRegisters();
                }
            }
            else if (selection == 6)//exit
            {
                running = false;
            }
        }
        cout<<output;

    }
    */
    SDL_DestroyWindow(window);
    SDL_Quit();
	
    return 0;
}

#pragma clang diagnostic pop
