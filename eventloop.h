#pragma once
#include <array>
#include <sys/epoll.h>

//#include "tcpconnection.h"

class TcpConnection;

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();
    EventLoop(EventLoop const&) = delete;
    EventLoop& operator=(EventLoop const&) = delete;

    bool init();
    bool run();
    int close();
    bool add(TcpConnection* conn);

private:
    int epoll_fd_;
    bool running_;
    static const int MaxEvents = 1024;
    static const int EpollTimeoutMs = 1000;
    std::array<epoll_event, MaxEvents> events_;
};
