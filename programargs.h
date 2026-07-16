#pragma once
#include <cstdint>
#include <string>

enum class SideEnum : char
{
    SideBuy = 'B',
    SideSell= 'S'
};



struct ProgramArgs
{
    bool ParseArgs(int argc, char** argv);

    std::string sym;
    SideEnum side;
    uint16_t vwapWinSizeSec; // vwap window size, seconds
    int32_t maxSize;
    std::string mdAddr;
    uint16_t mdPort;
    std::string orderEntryAddr;
    uint16_t orderEntryPort;
};