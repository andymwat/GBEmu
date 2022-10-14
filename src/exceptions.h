/*
	GBEmu - A simple DMG emulator
	Copyright (C) 2022 Andrew Watson

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
#ifndef GBEMU_EXCEPTIONS_H
#define GBEMU_EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include "fmt/format.h"

namespace exceptions
{
    class invalidOpcode : public std::runtime_error
    {
    public:
        explicit invalidOpcode(const std::string& message);
        explicit invalidOpcode(const std::string& message, uint16_t address);
        explicit invalidOpcode(const std::string& message, uint16_t address, uint8_t opcode);
    };
}


#endif //GBEMU_EXCEPTIONS_H
