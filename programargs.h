#pragma once
#include <cstdint>
#include <string>
#include <unistd.h>

#include string;

enum class SideEnum : char
{
    SideBuy = 'B',
    SideSell= 'S'
};

struct programargs
{
    std::string sym;
    SideEnum side;
    uint32_t maxSize;
    std::string mdAddr;
    uint16_t mdPort;
    std::string orderEntryAddr;
    uint16_t orderEntryPort;
};