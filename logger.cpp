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
    std::cout<<"Address: 0x"<<hex<<address<<dec<<" data: 0x"<<hex<<(uint16_t)data<<dec<<" "<<str<<endl;
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
    cout<<"Address: 0x"<<hex<<address<<dec<<" data: 0x"<<hex<<(uint16_t)data<<dec<<" "<<str<<endl;
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

