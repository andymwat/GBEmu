#
#    GBEmu - A simple DMG emulator
#    Copyright (C) 2020 Andrew Watson
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
cmake_minimum_required(VERSION 3.12)
project(GBEmu)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


if(WIN32)
    if (MSVC)  # Visual studio
        set(CMAKE_CXX_FLAGS_DEBUG "-Od")
        set(CMAKE_CXX_FLAGS_RELEASE "-Ox")
    else()
        set(CMAKE_CXX_FLAGS_DEBUG "-Og")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    endif()

    # Find SDL2
    set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")
    find_package(SDL2 REQUIRED COMPONENTS main)
    set(SDL_LIBRARY "${SDL2_DLLS}")
    set(SDL_INCLUDE_DIR "${SDL2_INCLUDE_DIRS}")
    find_package(SDL REQUIRED )
    if (MSVC)  # Visual studio
        add_executable(GBEmu WIN32 main.cpp main.h audioController.h audioController.cpp tests.cpp tests.h cartridge.cpp cartridge.h interpreter.cpp interpreter.h lcdController.cpp lcdController.h keyboardInput.cpp keyboardInput.h instructionDecoder.cpp bitIntstructionDecoder.cpp logger.cpp logger.h)
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
    else()  # MinGW
        add_executable(GBEmu main.cpp main.h audioController.h audioController.cpp tests.cpp tests.h cartridge.cpp cartridge.h interpreter.cpp interpreter.h lcdController.cpp lcdController.h keyboardInput.cpp keyboardInput.h instructionDecoder.cpp bitIntstructionDecoder.cpp logger.cpp logger.h)
    endif()

    INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(GBEmu ${SDL2_LIBS})
    #INCLUDE(FindPkgConfig)
    #PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    add_executable(GBEmu main.cpp main.h audioController.h audioController.cpp tests.cpp tests.h cartridge.cpp cartridge.h interpreter.cpp interpreter.h lcdController.cpp lcdController.h keyboardInput.cpp keyboardInput.h instructionDecoder.cpp bitIntstructionDecoder.cpp logger.cpp logger.h)
    INCLUDE(FindPkgConfig)
    PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
    INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(GBEmu ${SDL2_LIBRARIES})
endif()

if(WIN32)
    message("Copying dlls:")
    foreach(DLL ${SDL2_DLLS})
        add_custom_command(TARGET GBEmu POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:GBEmu>)
        message("\t ${DLL}")
    endforeach()
endif()