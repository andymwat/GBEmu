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
