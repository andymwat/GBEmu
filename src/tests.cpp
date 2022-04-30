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

#include "tests.h"


int runTest(cpuRegisterState startState, std::vector<uint8_t> instructions, cpuRegisterState expectedState)
{
	testInstructions = instructions.data();
	runningTest = true;
	a = startState.a;
	b = startState.b;
	c = startState.c;
	d = startState.d;
	e = startState.e;
	f = startState.f;
	h = startState.h;
	l = startState.l;
	pc = startState.pc;
	sp = startState.sp;

	m_TimerCounter = 1024;
	logger::logInfo("Starting test...");
	logger::logWarningNoData("WARNING: Instructions that reference memory WILL NOT WORK in tests. The address read from will be garbage.");


	try {
		for (uint16_t i = 0; i < instructions.size(); i++)
		{
			
				cycles = 0;
				execute(i);
				if (cycles == 0)
				{
					logger::logWarning("Cycle count not set.", pc, readFromAddress(pc));
				}
			
			
		}
	}
	catch (const char* msg)
	{
		std::cout << msg << std::endl;
		if (errorAddress >= 0)
		{
			logger::logError("Error accessing address, data could not be read (disregard data).", errorAddress, 0);
		}
		dumpRegisters();
	}
	startState.a = a;
	startState.b = b;
	startState.c = c;
	startState.d = d;
	startState.e = e;
	startState.f = f;
	startState.h = h;
	startState.l = l;
	startState.sp = sp;


	int result = expectedState.checkAgainst(startState);
	if (result != 0)
	{
		logger::logErrorNoData("Test(s) failed!");
	}
	else
	{
		logger::logInfo("Test(s) succeeded!");
	}

	runningTest = false;
	return result;
}