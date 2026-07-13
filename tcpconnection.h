#pragma once
#include <cstdint>
#include <string>
#include <memory>

struct sockaddr_in;

class TcpConnection
{
public:
    enum DuplexMode
    {
        HalfDuplex_In = 0x01,
        HalfDuplex_Out = 0x02,
        FullDuplex = HalfDuplex_In | HalfDuplex_Out
    };

    TcpConnection(TcpConnection const&) = delete;
    TcpConnection& operator=(TcpConnection const&) = delete;

    TcpConnection(std::string const& addr, uint16_t port, DuplexMode = FullDuplex);
    ~TcpConnection();
    int connect();
    int close();
    int getFd() const;
    DuplexMode getDuplexMode() const;
private:
    DuplexMode duplexMode_;
    int setNonBlocking();

    std::string addr_;
    uint16_t port_;

    int socketFd_;
    std::unique_ptr<sockaddr_in> serv_addr_;
};
