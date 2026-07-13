#include "eventloop.h"

#include "connectioncontext.h"
#include "exit_status.h"
#include "tcpconnection.h"

#include <iostream>

#include <sys/epoll.h>
#include <unistd.h>

EventLoop::EventLoop() :
epoll_fd_(-1),
running_(false)
{
    init();
}

EventLoop::~EventLoop()
{
    close();
}

bool EventLoop::init()
{
    if (running_ == true)
    {
        return false;
    }

    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1)
    {
        perror("epoll create failed");
        throw ProgramException(exit_status_fail);
    }

    return true;
}

bool EventLoop::run()
{
    if (running_ == true)
    {
        std::cout << "Eventloop already running\n";
        return false;
    }

    std::cout << "Eventloop starting...\n";
    running_ = true;

    while (true)
    {
        auto rst = epoll_wait(epoll_fd_, events_.data(), MaxEvents, EpollTimeoutMs);
        if (rst == -1)
        {
            if (errno == EINTR)
            {
                continue; // Interrupted; resume waiting.
            }
            else
            {
                perror("epoll wait failed");
                break;
            }
        }

        // loop over signaled fds
        for (int i = 0; i < rst; i++)
        {
            auto fd = events_[i].data.fd;
        }
    }

    return true;
}

int EventLoop::close()
{
    auto closeRst = ::close(epoll_fd_);
    if (closeRst != -1)
    {
        std::cout << "epoll closed\n";
    }
    else
    {
        perror("epoll close failed");
    }

    return closeRst;
}

bool EventLoop::add(TcpConnection* conn)
{
    epoll_event ev;
    ev.events = 0;

    TcpConnection::DuplexMode duplexMode = conn->getDuplexMode();

    if (duplexMode == TcpConnection::HalfDuplex_In || TcpConnection::HalfDuplex_Out || TcpConnection::FullDuplex)
    {
        if (duplexMode & TcpConnection::HalfDuplex_In)
        {
            ev.events |= EPOLLIN;
        }
        if (duplexMode & TcpConnection::HalfDuplex_Out)
        {
            ev.events |= EPOLLOUT;
        }
    }
    else
    {
        // default to full duplex
        ev.events |= EPOLLIN | EPOLLOUT;
    }

    ev.events |= EPOLLET;
    ev.data.ptr = static_cast<void*>(conn);
    auto ctlRst = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, conn->getFd(), &ev);
    if (ctlRst == -1)
    {
        perror("epoll ctl add failed");
        return false;
    }

    return true;
}

