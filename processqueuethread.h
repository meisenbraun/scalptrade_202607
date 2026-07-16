#pragma once
#include <thread>
#include "programargs.h"
#include "spscqueue.h"


struct PublishSignal;

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