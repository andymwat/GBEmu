#ifndef GBEMU_TESTS_H
#define GBEMU_TESTS_H

#include <vector>
#include "interpreter.h"
#include "logger.h"
#include "instructionDecoder.h"


//Run a series of instructions and check final cpu state against expected
int runTest(cpuRegisterState startState, std::vector<uint8_t> instructions, cpuRegisterState expectedState);

#endif