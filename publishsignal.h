#pragma once
#include <atomic>


struct PublishSignal
{
    long price;
    int32_t quoteSeqNum;
    int qty;

    std::atomic<bool> updated{false};
    std::atomic<bool> shutdown{false}; // command the SendOrderThread to shutdown
};