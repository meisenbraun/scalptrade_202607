#include "processqueuethread.h"
#include "market_data.h"
#include "publishsignal.h"

#include <algorithm>
#include <chrono>

#include <sched.h>
#include <iostream>
#include <immintrin.h>


ProcessQueueThread::ProcessQueueThread(SPSCQueue& queue) :
sym_(),
vwapIntervalSec_(0),
side_(SideEnum::SideBuy),
maxSize_(0),
thread_(),
queue_(queue),
vwap_numerator_(0L),
vwap_denominator_(0L),
last_vwap_(0L),
vwap_valid_(false),
pubSig_(nullptr)

{
    std::cout << "ProcessQueueThread starting...\n";

    pubSig_ = new PublishSignal();
}

ProcessQueueThread::~ProcessQueueThread()
{
    if (thread_.joinable())
    {
        thread_.join();
    }

    delete pubSig_;

    std::cout << "ProcessQueueThread exiting...\n";
}

bool ProcessQueueThread::setSym(std::string const& sym)
{
    if (sym.empty())
    {
        sym_ = sym;
        return true;
    }

    return false;
}

void ProcessQueueThread::setVwapInterval(uint16_t interval)
{
    vwapIntervalSec_ = interval;
}

void ProcessQueueThread::setSide(SideEnum side)
{
    side_ = side;
}

void ProcessQueueThread::setMaxSize(int32_t maxSize)
{
    maxSize_ = maxSize;
}

void ProcessQueueThread::run()
{
    // Set affinity mask
    cpu_set_t affinity_mask;
    CPU_SET(2, &affinity_mask); // Pin to core 2
    sched_setaffinity(0, sizeof(cpu_set_t), &affinity_mask);

    using clock = std::chrono::steady_clock;

    bool term=false;

    // track period in ms for better resolution
    const auto period = std::chrono::milliseconds(vwapIntervalSec_ * 1000);
    auto nextVwapSync = clock::now() + period;

    while (term == false)
    {
        QueueEvent ev = queue_.dequeue();
        switch (ev.msgType)
        {
        case MessageTypeQuote:
        {
            processQuote(std::move(ev.quote));
        }
        break;

        case MessageTypeTrade:
        {
            processTrade(std::move(ev.trade));
        }
        break;

        case MessageTypeNone: {_mm_pause();} break; // queue empty, continue
        case MessageTypeTerm: term = true; break;
        default:  // unknown event, skip
        {_mm_pause();}
        }

        auto now = clock::now();
        if (now >= nextVwapSync)
        {
            updateVwap();
            nextVwapSync = now + period;
        }
    }
}

bool ProcessQueueThread::processQuote(QuoteDataWire&& data)
{
    if (!vwap_valid_) {return false;}

    QuoteData quote;
    //quote.sym.assign(data.sym.data(), data.sym.size());

    try
    {
        quote.timestampNs = std::stoull(std::string(data.timestampNs.data(), data.timestampNs.size()));
    }
    catch (std::exception& e)
    {
        // parse error, skip the record
        return false;
    }

    try
    {
        quote.bidQty = std::stoi(std::string(data.bidQty.data(), data.bidQty.size()));
    }
    catch (std::exception& e)
    {
        return false;
    }

    try
    {
        quote.bidPrice = std::stoi(std::string(data.bidPrice.data(), data.bidPrice.size()));
    }
    catch (std::exception& e)
    {
        return false;
    }

    try
    {
        quote.askQty = std::stoi(std::string(data.askQty.data(), data.askQty.size()));
    }
    catch (std::exception& e)
    {
        return false;
    }

    try
    {
        quote.askPrice = std::stoi(std::string(data.askPrice.data(), data.askPrice.size()));
    }
    catch (std::exception& e)
    {
        return false;
    }

    switch (side_)
    {
        case SideEnum::SideBuy:
        {
            std::cout<<"Processing BUY quote (ask:" << quote.askPrice << " vwap: " << last_vwap_ << ")...\n";

            if (quote.askPrice < last_vwap_)
            {
                std::cout << "... Sending order!\n";

                // update params for order thread
                pubSig_->qty = std::max(quote.askQty, maxSize_);
                pubSig_->price = last_vwap_;
                pubSig_->updated.store(true, std::memory_order_release);
            }
            break;
        }
        case SideEnum::SideSell:
        {
            std::cout<<"Processing SELL quote (bid:" << quote.bidPrice << " vwap: " << last_vwap_ << ")...\n";

            if (quote.bidPrice > last_vwap_)
            {
                std::cout << "... Sending order!\n";

                // update params for order thread
                pubSig_->qty = std::max(quote.bidQty, maxSize_);
                pubSig_->price = last_vwap_;
                pubSig_->updated.store(true, std::memory_order_release);
            }
            break;
        }
    }

    return true;
}

bool ProcessQueueThread::processTrade(TradeDataWire&& data)
{
    TradeData trade;

    //trade.sym.assign(data.sym.data(), data.sym.size());

    try
    {
        trade.timestampNs = std::stoull(std::string(data.timestampNs.data(), data.timestampNs.size()));
    }
    catch (std::exception& e)
    {
        // parse error, skip the record

        std::cout << "ERROR1\n";
        return false;
    }

    try
    {
        trade.qty = std::stoi(std::string(data.qty.data(), data.qty.size()));
    }
    catch (std::exception& e)
    {
        std::cout << "ERROR2\n";
        return false;
    }

    try
    {
        trade.price = std::stoi(std::string(data.price.data(), data.price.size()));
    }
    catch (std::exception& e)
    {
        std::cout << "ERROR3\n";
        return false;
    }

    const int numerator = trade.price * trade.qty;
    vwap_numerator_ += numerator;

    const int denominator = trade.qty;
    vwap_denominator_ += denominator;

    std::cout << "Processing Trade (denominator:" << denominator << "  qty:" << trade.qty << ")...\n";

    return true;
}

PublishSignal* ProcessQueueThread::getPublishSignal()
{
    return pubSig_;
}

void ProcessQueueThread::start()
{
    thread_ = std::move(std::thread(&ProcessQueueThread::run, this));
}

void ProcessQueueThread::updateVwap()
{
    long newVwap = 0;

    std::cout <<"UPDATE VWAP TRIGGERED\n";

    if (vwap_denominator_ != 0)
    {

        newVwap = vwap_numerator_ / vwap_denominator_;
        std::cout << "UPDATING VWAP " << last_vwap_ << "  " << newVwap << std::endl;
        last_vwap_ = newVwap;
        vwap_valid_ = true;
    }
}


