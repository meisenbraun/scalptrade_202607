#include <iostream>
#include <memory>
#include "exit_status.h"
#include "programargs.h"
#include "tcpconnection.h"

void show_usage()
{
    std::cout << "Usage scalptrade_202607 <symbol> <Side> <Max size> <VWAP window, seconds> <MD TCP addr> <MD TCP port> <Order Entry TCP addr> <Order Entry port>\n\n";
    std::cout <<"Symbol must be at most 6 characters.\n";
    std::cout << "Side must be either \'B\' or \'S\'.\n";
}

void debug_args(int argc, char** argv)
{
    for (int i = 0; i < argc; ++i)
    {
        std::string arg = argv[i];
        std::cout << arg << "\n";
    }
}

int main(int argc, char** argv)
{
    if(argc != 9)
    {
        std::cout << "Incorrect number of arguments!\n";
        show_usage();
        return exit_status_args;
    }

    //debug_args(argc, argv);
    std::unique_ptr<ProgramArgs> args(new ProgramArgs);

    bool parseRst = args->ParseArgs(argc, argv);
    if (!parseRst)
    {
        show_usage();
        return exit_status_args;
    }

    std::unique_ptr<TcpConnection> mdServer(new TcpConnection(args->mdAddr, args->mdPort));
    mdServer->connect();

    return exit_status_success;
}