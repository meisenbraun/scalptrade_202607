#include <iostream>
#include "programargs.h"

void show_usage()
{
    std::cout << "Usage scalptrade_202607 <symbol> <Side> <Max size> <VWAP window, seconds> <MD TCP addr> <MD TCP port> <Order Entry TCP addr> <Order Entry port>\n\n";
    std::cout <<"Symbol must be at most 6 characters.\n";
    std::cout << "Side must be either \'B\' or \'S\'.\n";
}

int main(int argc, char** argv)
{
    if(argc != 9)
    {
        show_usage();
        return 3;
    }

    for (int i = 0; i < argc; ++i)
    {
        std::string arg = argv[i];
        std::cout << arg << "\n";
    }

    return 0;
}