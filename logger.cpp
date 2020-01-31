//
// Created by andrew on 1/31/20.
//

#include <iostream>
#include "logger.h"

void logger::logInfo(std::string str) {


#ifdef PLATFORM_UNIX
    std::cout<<"\033[1;32m[INFO]: \033[0m"<<str<<std::endl;
#else
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 15);
    std::cout<<"[INFO]: "<<str<<std::endl;
#endif

}

void logger::logError(std::string str, uint16_t address, uint8_t data) {

#ifdef PLATFORM_UNIX
    std::cout<<"\033[1;31m[ERROR]: \033[0m"<<str<<std::endl;
#else
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 12);
    std::cout<<"[ERROR]:";
    SetConsoleTextAttribute(hConsole, 15);
    std::cout<<"Address: 0x"<<hex<<address<<dec<<"\t data: 0x"<<hex<<(uint16_t)data<<dec<<"\t"<<str<<std::endl;
#endif

}

void logger::logWarning(std::string str, uint16_t address, uint8_t data) {


#ifdef PLATFORM_UNIX
    std::cout<<"\033[1;33m[WARNING]: \033[0m"<<str<<std::endl;
#else
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 12);
    std::cout<<"[WARNING]:";
    SetConsoleTextAttribute(hConsole, 15);
    std::cout<<"Address: 0x"<<hex<<address<<dec<<"\t data: 0x"<<hex<<(uint16_t)data<<dec<<"\t"<<str<<std::endl;
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

