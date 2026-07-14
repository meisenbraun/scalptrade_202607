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
    int getFd() const;
    bool isConnected() const;
    bool handleConnectionEstablished();
    int eventMask() const;
    void recv();
    std::string toString() const;
private:
    int setNonBlocking();
    bool init();

    static const int BufferSize_ = 256;
    std::string addr_;
    uint16_t port_;
    int socketFd_;
    std::unique_ptr<sockaddr_in> serv_addr_;
    bool connected_;
    int eventMask_;
};
