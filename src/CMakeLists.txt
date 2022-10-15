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

#Get other libraries (only fmt for now)
#set(OTHER_LIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/lib/include)
set(OTHER_LIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/fmt)


if(WIN32)
    if (MSVC)  # Visual studio
        message(STATUS "Detected MSVC compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
        set(CMAKE_CXX_FLAGS "-wd4068")
        set(CMAKE_CXX_FLAGS_DEBUG "-Od")
        set(CMAKE_CXX_FLAGS_RELEASE "-Ox")
    else()
        message(STATUS "Detected non-MSVC compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
        set(CMAKE_CXX_FLAGS_DEBUG "-Og -std=c++20")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -std=c++20")
    endif()

    # Find SDL2
    set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")
    find_package(SDL2 REQUIRED COMPONENTS main)
    set(SDL_LIBRARY "${SDL2_DLLS}")
    set(SDL_INCLUDE_DIR "${SDL2_INCLUDE_DIRS}")
    find_package(SDL REQUIRED)
    if (MSVC)  # Visual studio
        add_executable(GBEmu WIN32 main.cpp main.h audioController.h audioController.cpp tests.cpp tests.h cartridge.cpp cartridge.h interpreter.cpp interpreter.h lcdController.cpp lcdController.h keyboardInput.cpp keyboardInput.h instructionDecoder.cpp bitInstructionDecoder.cpp logger.cpp logger.h exceptions.cpp exceptions.h fmt/format.cc fmt/os.cc)
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
    else()  # MinGW
        add_executable(GBEmu main.cpp main.h audioController.h audioController.cpp tests.cpp tests.h cartridge.cpp cartridge.h interpreter.cpp interpreter.h lcdController.cpp lcdController.h keyboardInput.cpp keyboardInput.h instructionDecoder.cpp bitInstructionDecoder.cpp logger.cpp logger.h exceptions.cpp exceptions.h fmt/format.cc fmt/os.cc)
    endif()

    INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${OTHER_LIB_INCLUDE_DIR})
    #TARGET_LINK_LIBRARIES(GBEmu ${SDL2_LIBS} fmt)
    TARGET_LINK_LIBRARIES(GBEmu ${SDL2_LIBS})
    #INCLUDE(FindPkgConfig)
    #PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
else()
    message(STATUS "Detected compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unknown-pragmas")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    add_executable(GBEmu main.cpp main.h audioController.h audioController.cpp tests.cpp tests.h cartridge.cpp cartridge.h interpreter.cpp interpreter.h lcdController.cpp lcdController.h keyboardInput.cpp keyboardInput.h instructionDecoder.cpp bitInstructionDecoder.cpp logger.cpp logger.h exceptions.cpp exceptions.h fmt/format.cc fmt/os.cc)
    INCLUDE(FindPkgConfig)
    PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
    INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${OTHER_LIB_INCLUDE_DIR})
    #TARGET_LINK_LIBRARIES(GBEmu ${SDL2_LIBRARIES} fmt)
    TARGET_LINK_LIBRARIES(GBEmu ${SDL2_LIBRARIES})
endif()

if(WIN32)
    message("Copying dlls:")
    foreach(DLL ${SDL2_DLLS})
        add_custom_command(TARGET GBEmu POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:GBEmu>)
        message("\t ${DLL}")
    endforeach()
endif()
