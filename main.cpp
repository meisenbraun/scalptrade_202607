#include <iostream>
#include <memory>
#include "application.h"
#include "eventloop.h"
#include "exit_status.h"
#include "programargs.h"
#include "tcpconnection.h"

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
    bool retval = exit_status_success;

    std::unique_ptr<Application> app(new Application(argc, argv));

    app->init();
    app->run();
}

#if 0
int main(int argc, char** argv)
{
    bool retval = exit_status_success;

    std::unique_ptr<EventLoop> eventLoop;


    try
    {
        if(argc != 9)
        {
            std::cout << "Incorrect number of arguments!\n";
            show_usage();
            throw ProgramException(exit_status_args);
        }

        //debug_args(argc, argv);
        std::unique_ptr<ProgramArgs> args(new ProgramArgs);

        bool parseRst = args->ParseArgs(argc, argv);
        if (!parseRst)
        {
            show_usage();
            throw ProgramException(exit_status_args);
        }

        eventLoop.reset(new EventLoop());

        std::unique_ptr<TcpConnection> mdServer(new TcpConnection(args->mdAddr, args->mdPort));
        bool mdAddRst = eventLoop->add(mdServer.get());
        if (mdAddRst == false)
        {
            throw ProgramException(exit_status_fail);
        }

        std::unique_ptr<TcpConnection> orderEntry(new TcpConnection(args->orderEntryAddr, args->orderEntryPort));
        bool oeAddRst = eventLoop->add(orderEntry.get());
        if (oeAddRst == false)
        {
            throw ProgramException(exit_status_fail);
        }

        mdServer->connect();
        orderEntry->connect();
        eventLoop->run();

        std::cout << "Event loop exit\n";
    }
    catch (ProgramException &e)
    {
        retval = e.code();
    }

    std::cout << "Program exiting with code " <<  retval << "\n";

    return retval;
}
#endif