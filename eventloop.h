#pragma once
#include <array>
#include <sys/epoll.h>

#include "application.h"

class TcpConnection;

class EventLoop
{
public:
    EventLoop(EventLoop const&) = delete;
    EventLoop& operator=(EventLoop const&) = delete;

    EventLoop(Application& application);
    ~EventLoop();

    bool init();
    bool run();
    int close();
    bool add(TcpConnection* conn);

private:
    Application& app_;
    int epoll_fd_;
    bool running_;
    static const int MaxEvents = 1024;
    static const int EpollTimeoutMs = 10000;
    std::array<epoll_event, MaxEvents> events_;
};
