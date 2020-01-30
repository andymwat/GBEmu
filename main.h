//
// Created by andrew on 10/12/19.
//
#include <string>
#include <sstream>


#ifndef GBEMU_MAIN_H
#define GBEMU_MAIN_H

template <typename T>
std::string to_string(T value)
{
    std::ostringstream os ;
    os << value ;
    return os.str() ;
}

#endif //GBEMU_MAIN_H
