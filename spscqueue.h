#pragma once
#include <atomic>
#include <optional>
//#include <variant>
#include "market_data.h"

//using QueueEvent = std::variant<QuoteDataWire, TradeDataWire>;

class SPSCQueue
{
public:

    SPSCQueue(SPSCQueue const &) = delete;
    SPSCQueue& operator=(SPSCQueue const &) = delete;
    SPSCQueue();
    bool enqueue(const QueueEvent&& ev);
    QueueEvent dequeue();

private:

    static constexpr int Capacity_ = 2048; // must be power of 2;
    static_assert((Capacity_ & (Capacity_ - 1)) == 0, "Capacity must be a power of 2");
    static constexpr int Mask_ = Capacity_ - 1;


    QueueEvent buffer_[Capacity_];
    alignas(64) std::atomic<std::size_t> head_; // align on cacheline size to prevent false sharing
    alignas(64) std::atomic<std::size_t> tail_;

    alignas(64) std::size_t cachedHead_;
    alignas(64) std::size_t cachedTail_;

};
