cmake_minimum_required(VERSION 3.14)
project(GBEmu)

set(CMAKE_CXX_STANDARD 17)

add_executable(GBEmu main.cpp main.h cartridge.cpp cartridge.h interpreter.cpp interpreter.h lcdController.cpp lcdController.h keyboardInput.cpp keyboardInput.h instructionDecoder.cpp bitIntstructionDecoder.cpp)

INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(GBEmu ${SDL2_LIBRARIES} )
