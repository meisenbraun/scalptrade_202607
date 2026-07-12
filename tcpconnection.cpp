#include "tcpconnection.h"
#include "exit_status.h"
#include <iostream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

TcpConnection::TcpConnection(std::string const& addr, uint16_t port) :
addr_(addr),
port_(port),
socketFd_(-1)
{}

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

int TcpConnection::connect()
{
    socketFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd_ < 0)
    {
        perror("Unable to establish connection");
        exit(exit_status_fail);
    }

    serv_addr_.reset(new sockaddr_in);
    serv_addr_->sin_family = AF_INET;
    serv_addr_->sin_port = htons(port_);

    // Dotted-quad string flat int
    auto pton_rst = inet_pton(AF_INET, addr_.c_str(), &serv_addr_->sin_addr);
    if (pton_rst <= 0)
    {
        perror("Unable to resolve IP address");
        exit(exit_status_fail);
    }

    auto connectRst = ::connect(socketFd_, (sockaddr*)(serv_addr_.get()), sizeof(sockaddr_in));

    return 0;
}

int TcpConnection::close()
{
    return 0;
}
