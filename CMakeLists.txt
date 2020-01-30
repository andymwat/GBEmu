cmake_minimum_required(VERSION 3.12)
project(GBEmu)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)


# Find SDL2
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")
find_package(SDL2 REQUIRED COMPONENTS main)
set(SDL_LIBRARY "${SDL2_DLLS}")
set(SDL_INCLUDE_DIR "${SDL2_INCLUDE_DIRS}")
find_package(SDL REQUIRED )
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
add_executable(GBEmu WIN32 main.cpp main.h cartridge.cpp cartridge.h interpreter.cpp interpreter.h lcdController.cpp lcdController.h keyboardInput.cpp keyboardInput.h instructionDecoder.cpp bitIntstructionDecoder.cpp)
TARGET_LINK_LIBRARIES(GBEmu ${SDL2_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
#INCLUDE(FindPkgConfig)
#PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)

if(WIN32)
	message("Copying dlls:")
    foreach(DLL ${SDL2_DLLS})
        add_custom_command(TARGET GBEmu POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:GBEmu>)
		message(${DLL})
    endforeach()	
endif()