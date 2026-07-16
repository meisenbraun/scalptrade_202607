#pragma once
#include <atomic>


struct PublishSignal
{
    long price;
    int qty;

    std::atomic<bool> updated{false};
    std::atomic<bool> shutdown{false};
};