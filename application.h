#pragma once
#include <memory>


class ProgramArgs;
class EventLoop;
class TcpConnection;

class Application
{
public:
    Application(int argc, char** argv);
    Application(Application& app) = delete;
    Application& operator=(Application& app) = delete;

    void init();
    int run();

private:
    void show_usage();

    std::unique_ptr<ProgramArgs> args_;
    std::unique_ptr<EventLoop> eventLoop_;
    std::unique_ptr<TcpConnection> mdConnection_;
    std::unique_ptr<TcpConnection> orderEntryConnection_;
};