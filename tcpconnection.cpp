#include "tcpconnection.h"
#include "exit_status.h"
#include <iostream>
#include <sstream>

#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>

TcpConnection::TcpConnection(std::string const& addr, uint16_t port) :
addr_(addr),
port_(port),
socketFd_(-1),
connected_(false),
eventMask_(0)
{
    init();
}

TcpConnection::~TcpConnection()
{
    close();
}

int TcpConnection::setNonBlocking()
{
    int flg = fcntl(socketFd_, F_GETFL, 0);
    if (flg == -1) return -1;
    return fcntl(socketFd_, F_SETFL, flg | O_NONBLOCK);
}

bool TcpConnection::init()
{
    socketFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd_ < 0)
    {
        perror("Unable to establish connection");
        throw ProgramException(exit_status_fail); // causes stack unwinding
    }

    eventMask_ = EPOLLIN | EPOLLOUT | EPOLLET; // initial event mask

    return true;
}

int TcpConnection::connect()
{

    auto nonBlockingRst = setNonBlocking();
    if (nonBlockingRst == -1)
    {
        perror("Unable to set non blocking");
        throw ProgramException(exit_status_fail);
    }

    serv_addr_.reset(new sockaddr_in);
    serv_addr_->sin_family = AF_INET;
    serv_addr_->sin_port = htons(port_);

    // Dotted-quad string flat int for the API
    auto pton_rst = inet_pton(AF_INET, addr_.c_str(), &serv_addr_->sin_addr);
    if (pton_rst <= 0)
    {
        perror("Unable to resolve IP address");
        throw ProgramException(exit_status_fail);
    }

    std::cout << "Connecting to " << toString() << "...\n";

    auto connectRst = ::connect(socketFd_, (sockaddr*)(serv_addr_.get()), sizeof(sockaddr_in));

    return 0;
}

int TcpConnection::close()
{
    int closeRst = ::close(socketFd_);
    if (closeRst != -1)
    {
        std::cout << "Socket " << toString() << " closed\n";
    }
    else
    {
        perror("Unable to close socket");
    }

    return closeRst;
}

int TcpConnection::getFd() const
{
    return socketFd_;
}

bool TcpConnection::isConnected() const
{
    return connected_;
}

bool TcpConnection::handleConnectionEstablished()
{
    if (connected_)
    {
        return false; // idempotency protection
    }

    int err = 0;
    socklen_t len = sizeof(err);
    auto errRst = getsockopt(socketFd_, SOL_SOCKET, SO_ERROR, &err, &len);
    if (errRst < 0)
    {
        perror("getsockopt failed");
        throw ProgramException(exit_status_fail);
    }

    if (err != 0)
    {
        std::cerr << "Connection to " << toString() << " failed: ", strerror(err);
        throw ProgramException(exit_status_fail);
    }


    connected_ = true;
    std::cout << "Connection to " << toString() << " established\n";

    eventMask_ &= ~EPOLLOUT; // clear EPOLL

    return true;
}

void TcpConnection::recv()
{
    char readBuffer[BufferSize_];

    // while loop required to drain recv buffer for ET mode
    while (true)
    {
        auto recvRst = ::recv(socketFd_, readBuffer, BufferSize_ - 1, 0);

        if (recvRst > 0)
        {
            readBuffer[recvRst] = '\0';
        }
        else if (recvRst == 0)
        {
            std::cout << "Connection " << toString() << " closed by peer\n";
            connected_ = false;
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // end of buffer; not an error in this context
            }
            else
            {
                perror("Error reading socket");
                connected_ = false;
                break;
            }
        }
    }
}

int TcpConnection::eventMask() const
{
    return eventMask_;
}

std::string TcpConnection::toString() const
{
    std::ostringstream ostrm;

    ostrm << addr_ << ":" << port_;

    return ostrm.str();
}