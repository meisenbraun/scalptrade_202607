#include "eventloop.h"

#include "exit_status.h"
#include "tcpconnection.h"

#include <iostream>

#include <sys/epoll.h>
#include <unistd.h>

EventLoop::EventLoop(Application& application) :
app_(application),
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
    else
    {
        std::cout << "epoll created\n";
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
            // handle connection established (SYN-ACK)
            TcpConnection* conn = static_cast<TcpConnection*>(events_[i].data.ptr);
            const bool isConnected = conn->isConnected();
            if (!isConnected && events_[i].events & EPOLLOUT)
            {
                const bool connRst  = conn->handleConnectionEstablished();
                if (connRst)
                {
                    // reset the event mask to clear EPOLLOUT
                    events_[i].events = conn->eventMask();
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn->getFd(), &events_[i]);
                }
                continue;
            }

            // process incoming data
            if (events_[i].events & EPOLLIN)
            {
                conn->recv(app_.getSPSCQueue());
                if (conn->isConnected() == false)
                {
                    // connection dropped, cleanup the object
                    delete conn;
                }
            }
            if (events_[i].events & EPOLLOUT)
            {
                conn->resumeSend(); // retry a blocked send
                if (conn->isConnected())
                {
                    // reset the event mask to clear EPOLLOUT
                    events_[i].events = conn->eventMask();
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn->getFd(), &events_[i]);
                }
                else
                {
                    delete conn;
                }
            }
        }
    }

    std::cout << "EventLoop exit\n";

    return true;
}

int EventLoop::close()
{
    app_.getSPSCQueue().enqueue(QueueEvent{MessageTypeTerm}); // Signal worker thread to shutdown

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

    ev.events = conn->eventMask();
    ev.data.ptr = static_cast<void*>(conn);
    auto ctlRst = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, conn->getFd(), &ev);
    if (ctlRst == -1)
    {
        perror("epoll ctl add failed");
        return false;
    }

    return true;
}

