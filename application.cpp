#include "application.h"

#include "eventloop.h"
#include "programargs.h"
#include "tcpconnection.h"
#include "exit_status.h"

#include <iostream>

Application::Application(int argc, char** argv)
{
    if(argc != 9)
    {
        std::cout << "Incorrect number of arguments!\n";
        show_usage();
        throw ProgramException(exit_status_args);
    }

    //debug_args(argc, argv);
    args_.reset(new ProgramArgs);

    bool parseRst = args_->ParseArgs(argc, argv);
    if (!parseRst)
    {
        show_usage();
        throw ProgramException(exit_status_args);
    }

}


void Application::show_usage()
{
    std::cout << "Usage scalptrade_202607 <symbol> <Side> <Max size> <VWAP window, seconds> <MD TCP addr> <MD TCP port> <Order Entry TCP addr> <Order Entry port>\n\n";
    std::cout <<"Symbol must be at most 6 characters.\n";
    std::cout << "Side must be either \'B\' or \'S\'.\n";
}

void Application::init()
{
}

void Application::run()
{
    eventLoop_.reset(new EventLoop());

    std::unique_ptr<TcpConnection> mdServer(new TcpConnection(args_->mdAddr, args_->mdPort, *this));
    bool mdAddRst = eventLoop_->add(mdServer.get());
    if (mdAddRst == false)
    {
        throw ProgramException(exit_status_fail);
    }

    std::unique_ptr<TcpConnection> orderEntry(new TcpConnection(args_->orderEntryAddr, args_->orderEntryPort, *this));
    bool oeAddRst = eventLoop_->add(orderEntry.get());
    if (oeAddRst == false)
    {
        throw ProgramException(exit_status_fail);
    }

    mdServer->connect();
    orderEntry->connect();
    eventLoop_->run();

    std::cout << "Application exit\n";
}

SPSCQueue& Application::getSPSCQueue()
{
    return eventQueue_;
}