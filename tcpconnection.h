#pragma once
#include <cstdint>
#include <string>
#include <memory>

struct sockaddr_in;


class TcpConnection
{
public:
    TcpConnection(TcpConnection const&) = delete;
    TcpConnection& operator=(TcpConnection const&) = delete;

    TcpConnection(std::string const& addr, uint16_t port);
    ~TcpConnection();
    int connect();
    int close();


private:
    int setNonBlocking();

    std::string addr_;
    uint16_t port_;

    int socketFd_;
    std::unique_ptr<sockaddr_in> serv_addr_;
};
