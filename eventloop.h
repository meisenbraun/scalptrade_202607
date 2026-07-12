#pragma once

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();
    EventLoop(EventLoop const&) = delete;
    EventLoop& operator=(EventLoop const&) = delete;

    int init();
    int run();

private:
    int epoll_fd_;
};