//
// Created by andrew on 1/31/20.
//

#ifndef GBEMU_LOGGER_H
#define GBEMU_LOGGER_H


#include <string>

#if defined(unix) || defined(__unix__) || defined(__unix)
#define PLATFORM_UNIX
#endif

#if defined(_WIN32)
#define PLATFORM_WINDOWS
#include <windows.h> // WinApi header
#endif

class logger {
public:
    static void logInfo(std::string str);
    static void logError(std::string str,  uint16_t address, uint8_t data);
    static void logWarning(std::string str,  uint16_t address, uint8_t data);
    static void logErrorNoData(std::string str);
	static void logWarningNoData(std::string str);
};


#endif //GBEMU_LOGGER_H
