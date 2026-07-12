#include "programargs.h"
#include <iostream>
#include <string>

bool charToSide(char c, SideEnum & e)
{
    auto C = toupper(c);
    switch (C)
    {
        case 'B' : e = SideEnum::SideBuy; return true;
        case 'S'  : e = SideEnum::SideSell; return true;
        default:
        return false;
    }
}


bool ProgramArgs::ParseArgs(int argc, char** argv)
{
    sym = argv[1];
    if (sym.size() > 6)
    {
        std::cout << "symbol is too long!\n";
        return false;
    }

    auto sideRst = charToSide(argv[2][0], side);
    if (!sideRst)
    {
        std::cout << "invalid side\n";
        return false;
    }

    try
    {
        maxSize = std::stoi(argv[3]);
    }catch (std::exception&)
    {
        std::cout << "invalid max size\n";
        return false;
    }

    try
    {
        vwapWinSizeSec = std::stoi(argv[4]);
    }
    catch (std::exception&)
    {
        std::cout << "Invalid VWAP Window size\n";
        return false;
    }


    mdAddr = argv[5];
    try
    {
        mdPort = std::stoi(argv[6]);
    }
    catch (std::exception&)
    {
        std::cout << "Invalid md port\n";
        return false;
    }

    orderEntryAddr = argv[7];
    try
    {
        orderEntryPort = std::stoi(argv[8]);
    }
    catch (std::exception&)
    {
        std::cout << "invalid order entry port\n";
        return false;
    }

    return true;
}