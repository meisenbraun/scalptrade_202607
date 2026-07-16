#pragma once
#include <memory>

#include "processqueuethread.h"
#include "sendorderthread.h"
#include "spscqueue.h"


class ProgramArgs;
class EventLoop;
class TcpConnection;

class Application
{
public:
    Application(int argc, char** argv);
    ~Application();

    Application(Application& app) = delete;
    Application& operator=(Application& app) = delete;

    void init();
    void run();
    SPSCQueue& getSPSCQueue();

private:
    void show_usage();
    SPSCQueue eventQueue_;
    std::unique_ptr<ProgramArgs> args_;
    std::unique_ptr<EventLoop> eventLoop_;
    std::unique_ptr<TcpConnection> mdConnection_;
    std::unique_ptr<TcpConnection> orderEntryConnection_;
    ProcessQueueThread processQueueThread_;
    SendOrderThread sendOrderThread_;
};