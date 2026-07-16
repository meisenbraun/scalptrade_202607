#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include "market_data.h"

struct sockaddr_in;
class Application;

class TcpConnection
{
public:
    TcpConnection(TcpConnection const&) = delete;
    TcpConnection& operator=(TcpConnection const&) = delete;

    TcpConnection(std::string const& addr, uint16_t port, Application& app);
    ~TcpConnection();
    int connect();
    int close();
    int getFd() const;
    bool isConnected() const;
    bool handleConnectionEstablished();
    int eventMask() const;
    void recv();
    bool send(void* data, int dataLen);
    bool resumeSend();
    std::string toString() const;
private:
    int setNonBlocking();
    bool init();

    static const int BufferSize_ = sizeof(QuoteDataWire) * 10;
    char writeBuffer_[sizeof(OrderDataWire) * 2];
    int writeBufferSz_;

    std::string addr_;
    uint16_t port_;
    int socketFd_;
    std::unique_ptr<sockaddr_in> serv_addr_;
    bool connected_;
    int eventMask_;
};
