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
// Created by andrew on 1/31/20.
//

#include <iostream>
#include "logger.h"


using namespace std;

void logger::logInfo(std::string str) {


#ifdef PLATFORM_UNIX
    std::cout<<"\033[1;32m[INFO]: \033[0m"<<str<<std::endl;
#else
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 1);
	std::cout << "[INFO]: ";
	SetConsoleTextAttribute(hConsole, 8);
	std::cout << str << std::endl;

	SetConsoleTextAttribute(hConsole, 15);
	
#endif

}

void logger::logError(std::string str, uint16_t address, uint8_t data) {

#ifdef PLATFORM_UNIX
    std::cout<<"\033[1;31m[ERROR]: \033[0m"<<str<<std::endl;
    std::cout<<"Address: 0x"<<hex<<address<<dec<<" data: 0x"<<hex<<(uint16_t)data<<dec<<endl;
#else
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 12);
    cout<<"[ERROR]:";
    SetConsoleTextAttribute(hConsole, 15);
    cout<<"Address: 0x"<<hex<<address<<dec<<" data: 0x"<<hex<<(uint16_t)data<<dec<<" "<<str<<endl;
#endif

}

void logger::logWarning(std::string str, uint16_t address, uint8_t data) {


#ifdef PLATFORM_UNIX
    std::cout<<"\033[1;33m[WARNING]: \033[0m"<<str<<std::endl;
    cout<<"Address: 0x"<<hex<<address<<dec<<" data: 0x"<<hex<<(uint16_t)data<<dec<<endl;
#else
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 14);
    std::cout<<"[WARNING]:";
    SetConsoleTextAttribute(hConsole, 15);
    std::cout<<"Address: 0x"<<hex<<address<<dec<<" data: 0x"<<hex<<(uint16_t)data<<dec<<" "<<str<<std::endl;
#endif

}

void logger::logErrorNoData(std::string str) {
#ifdef PLATFORM_UNIX
    std::cout<<"\033[1;31m[ERROR]: \033[0m"<<str<<std::endl;
#else
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 12);
    std::cout<<"[ERROR]:";
    SetConsoleTextAttribute(hConsole, 15);
    std::cout<<str<<std::endl;
#endif

}

void logger::logWarningNoData(std::string str)
{
#ifdef PLATFORM_UNIX
	std::cout << "\033[1;31m[Warning]: \033[0m" << str << std::endl;
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, 14);
	std::cout << "[Warning]:";
	SetConsoleTextAttribute(hConsole, 15);
	std::cout << str << std::endl;
#endif
}

