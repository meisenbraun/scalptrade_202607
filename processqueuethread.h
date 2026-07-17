#pragma once
#include "programargs.h"
#include "spscqueue.h"
#include "publishsignal.h"

#include <atomic>
#include <thread>

#include <immintrin.h>

class ProcessQueueThread
{
public:
    ProcessQueueThread(ProcessQueueThread& processQueueThread) = delete;
    ProcessQueueThread& operator= (ProcessQueueThread& processQueueThread) = delete;

    ProcessQueueThread(SPSCQueue& queue);
    ~ProcessQueueThread();

    bool setSym(std::string const& sym);
    void setVwapInterval(uint16_t interval);
    void setSide(SideEnum side);
    void setMaxSize(int32_t maxSize);
    PublishSignal* getPublishSignal();

    void start();


private:
    void run();
    bool processQuote(QuoteDataWire&& data);
    bool processTrade(TradeDataWire&& data);
    void updateVwap();
    void updatePublishSignal(int32_t qty, int32_t price, int32_t seqNum);

    // Sym is invariant for a given data feed.
    // This application only supports a single data feed,
    // So this value is initialized from program arguments
    std::string sym_;
    uint16_t vwapIntervalSec_; // also invariant
    SideEnum side_;            //
    int32_t maxSize_;          //

    std::thread thread_;
    SPSCQueue& queue_;

    long vwap_numerator_;
    long vwap_denominator_;

    long last_vwap_;
    bool vwap_valid_;
    PublishSignal* pubSig_; // For communicating with the sending thread

};

__attribute__((always_inline))
inline void ProcessQueueThread::updatePublishSignal(int32_t qty, int32_t price, int32_t seqNum)
{
    // spin until the order entry thread has cleard the flag
    // prevents the PublishSignal struct from being written while read
    while (pubSig_->updated.load(std::memory_order_acquire))
    {
        _mm_pause();
    }

    // update params for order thread
    pubSig_->qty = std::min(qty, maxSize_);
    pubSig_->price = price;
    pubSig_->quoteSeqNum = seqNum;

    pubSig_->updated.store(true, std::memory_order_release);
}
