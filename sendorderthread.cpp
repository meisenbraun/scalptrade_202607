#include "sendorderthread.h"

#include "market_data.h"
#include "programargs.h"
#include "publishsignal.h"

#include <algorithm>
#include <chrono>
#include <iostream>

#include <immintrin.h>
#include <sched.h>

SendOrderThread::SendOrderThread() :
thread_(),
pubSig_(nullptr),
sym_(),
maxSize_()
{
}

SendOrderThread::~SendOrderThread()
{
    if (thread_.joinable())
    {
        thread_.join();
    }

    std::cout << "SendOrderThread exiting...\n";
}

void SendOrderThread::setPublishSignal(PublishSignal* pubSig)
{
    pubSig_ = pubSig;
}

void SendOrderThread::setSym(const std::string& sym)
{
    sym_ = sym;
}

void SendOrderThread::setMaxSize(int32_t maxSize)
{
    maxSize_ = maxSize;
}

void SendOrderThread::setSide(SideEnum side)
{
    side_ = side;
}

void SendOrderThread::start()
{
    std::cout << "Order Sender thread starting...\n";
    thread_ = std::move(std::thread(&SendOrderThread::run, this));
}

void SendOrderThread::run()
{
    using clock = std::chrono::system_clock;

    // Set affinity mask
    cpu_set_t affinity_mask;
    CPU_SET(3, &affinity_mask); // Pin to core 3
    sched_setaffinity(0, sizeof(cpu_set_t), &affinity_mask);


    OrderDataWire ord;

    // initialize the invariants
    std::size_t i = 0;
    for (; i < ord.sym.size() && i < sym_.size(); i++)
    {
        ord.sym[i] = sym_[i];
    }
    for (; i < ord.sym.size(); ++i)
    {
        ord.sym[i] = '\0'; // pad with nulls;
    }

    switch (side_)
    {
    case SideEnum::SideBuy:
    {
        ord.side = {'B','U','Y','\0'}; // null padded
    }
    break;
    case SideEnum::SideSell:
        ord.side = {'S', 'E', 'L', 'L'};
    }

    //bool updated = false;
    bool shutdown = false;

    while (true)
    {
        // poll until data is ready
        while (!(pubSig_->updated.load(std::memory_order_acquire)) &&
               !(shutdown = pubSig_->shutdown.load(std::memory_order_relaxed)))
        {
            _mm_pause();
        }

        if (shutdown)
        {
            break;
        }

        long myPrice = pubSig_->price;
        int myQty = pubSig_->qty;

        pubSig_->updated.store(false, std::memory_order_release);

        auto nanoNow = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count();
        formatTimestamp(nanoNow, ord.timestampNs);

        formatQtyAndPrice(myQty, ord.qty);
        formatQtyAndPrice(myPrice, ord.price);


        /// send order
    }
}

// Avoids extra copies that would be needed if
// std::to_string was used. Assumes val is a positive int
// representing nanos since epoch. Assumes exactly 19 digits
bool SendOrderThread::formatTimestamp(int64_t val, std::array<char, 19>& out) const
{
    if (val < 0) {return false;}

    for (auto iter = out.rbegin(); iter != out.rend(); ++iter)
    {
        *iter = static_cast<char>('0' + (val % 10));
        val /= 10;
    }

    return val == 0; // false if val is more than 19 digits
}


// Slight variation on the above
bool SendOrderThread::formatQtyAndPrice(int32_t val, std::array<char, 6>& out) const
{
    if (val < 0) {return false;}

    for (auto iter = out.rbegin(); iter != out.rend(); ++iter)
    {
        *iter = static_cast<char>('0' + (val % 10));
        val /= 10;
    }

    return val == 0; // false if val is more than 6 digits
}

void SendOrderThread::setTcpConnection(TcpConnection* conn)
{
    tcpConnection_ = conn;
}


