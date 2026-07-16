#include "application.h"

#include "eventloop.h"
#include "exit_status.h"
#include "processqueuethread.h"
#include "programargs.h"
#include "tcpconnection.h"

#include <sched.h>

#include <iostream>

Application::Application(int argc, char** argv) :
eventQueue_(),
processQueueThread_(eventQueue_),
sendOrderThread_()
{
    if(argc != 9)
    {
        std::cout << "Incorrect number of arguments!\n";
        show_usage();
        throw ProgramException(exit_status_args);
    }

    args_.reset(new ProgramArgs);

    bool parseRst = args_->ParseArgs(argc, argv);
    if (!parseRst)
    {
        show_usage();
        throw ProgramException(exit_status_args);
    }
}

Application::~Application()
{
    eventQueue_.enqueue(QueueEvent{MessageTypeTerm});
}


void Application::show_usage()
{
    std::cout << "Usage scalptrade_202607 <symbol> <Side> <Max size> <VWAP window, seconds> <MD TCP addr> <MD TCP port> <Order Entry TCP addr> <Order Entry port>\n\n";
    std::cout <<"Symbol must be at most 6 characters.\n";
    std::cout << "Side must be either \'B\' or \'S\'.\n";
}

void Application::init()
{
    // Set affinity mask
    cpu_set_t affinity_mask;
    CPU_SET(1, &affinity_mask); // Pin to core 1
    sched_setaffinity(0, sizeof(cpu_set_t), &affinity_mask);


    processQueueThread_.setSym(args_->sym);
    processQueueThread_.setVwapInterval(args_->vwapWinSizeSec);
    processQueueThread_.setSide(args_->side);
    processQueueThread_.setMaxSize(args_->maxSize);
    sendOrderThread_.setSym(args_->sym);
    sendOrderThread_.setMaxSize(args_->maxSize);
    sendOrderThread_.setSide(args_->side);
    sendOrderThread_.setPublishSignal(processQueueThread_.getPublishSignal());
    sendOrderThread_.setTcpConnection(orderEntryConnection_.get());
}

void Application::run()
{
    processQueueThread_.start();
    sendOrderThread_.start();

    eventLoop_.reset(new EventLoop(*this));

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