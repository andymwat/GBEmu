/*
 *     GBEmu - A simple DMG emulator
 *     Copyright (C) 2022 Andrew Watson
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef GBEMU_EXCEPTIONS_H
#define GBEMU_EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include "fmt/format.h"

namespace exceptions
{
    /**
     * @brief An exception that is thrown when an invalid opcode is encountered.
     */
    class invalidOpcode : public std::runtime_error
    {
    public:
        /**
         * @brief Construct a new invalid opcode object
         * @param message The message to be displayed when the exception is thrown.
         */
        explicit invalidOpcode(const std::string& message);

        /**
         * @brief Construct a new invalid opcode object
         * @param message The message to be displayed when the exception is thrown.
         * @param address The address of the invalid opcode.
         */
        explicit invalidOpcode(const std::string& message, uint16_t address);

        /**
         * @brief Construct a new invalid opcode object
         * @param message The message to be displayed when the exception is thrown.
         * @param address The address of the invalid opcode.
         * @param opcode The invalid opcode.
         */
        explicit invalidOpcode(const std::string& message, uint16_t address, uint8_t opcode);
    };

    /**
     * @brief An exception for SDL errors.
     */
    class SDLException : public std::runtime_error {
    public:
        /**
         * @brief Construct a new SDL exception object
         * @param message The message to be displayed when the exception is thrown.
         */
        explicit SDLException(const std::string& message);
    };

    /**
     * @brief An exception for invalid instructions/opcodes.
     */
    class invalidInstruction : public std::runtime_error {
    public:
        explicit invalidInstruction(const std::string& message);
    };

    /**
     * @brief An exception for errors relating to the cartridge
     */
    class cartridgeError : public std::runtime_error {
    public:
        explicit cartridgeError(const std::string& message);
        explicit cartridgeError(const std::string& message, uint8_t data);
        explicit cartridgeError(const std::string& message, uint16_t data);
    };
}


#endif //GBEMU_EXCEPTIONS_H
