#pragma once
#include <thread>

#include "programargs.h"
#include "tcpconnection.h"

struct PublishSignal;

class SendOrderThread
{
public:
    SendOrderThread(SendOrderThread const&) = delete;
    SendOrderThread& operator=(SendOrderThread const&) = delete;
    SendOrderThread();
    ~SendOrderThread();

    void setPublishSignal(PublishSignal* pubSig);
    void setSym(const std::string& sym);
    void setMaxSize(int32_t maxSize);
    void setSide(SideEnum side);
    void setTcpConnection(TcpConnection* tcpConnection);
    void start();

private:
    void run();
    bool formatTimestamp(int64_t val, std::array<char, 19>& out) const;
    bool formatQtyAndPrice(int32_t val, std::array<char, 6>& out) const;
    bool formatSeq(int32_t val, std::array<char, 7>& out) const;

    std::thread thread_;

    PublishSignal* pubSig_; // does not take ownership
    TcpConnection* tcpConnection_; // does not take ownership

    // These fields are invariant
    std::string sym_;
    SideEnum side_;
    uint32_t maxSize_;
};
