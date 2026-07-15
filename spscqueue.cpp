#include "spscqueue.h"

SPSCQueue::SPSCQueue() :
head_(0),
tail_(0),
cachedHead_(0),
cachedTail_(0)
{

}

bool SPSCQueue::enqueue(const QueueEvent&& ev)
{
    // cheap load first
    const std::size_t currentTail = tail_.load(std::memory_order_relaxed);

    // check if full
    if ((currentTail - cachedHead_) == Capacity_)
    {
        // Expensive load only if first checks show at capacity
        cachedHead_ = head_.load(std::memory_order_acquire);

        // verify if really full
        if (currentTail - cachedHead_ == Capacity_)
        {
            return false; // no room at the inn
        }
    }

    // cheap modulus substitute. Only works if Capacity_ is power of 2.
    buffer_[currentTail & Mask_] = std::move(ev);

    // mark the slot taken
    tail_.store(currentTail + 1, std::memory_order_release);

    return true;
}

QueueEvent SPSCQueue::dequeue()
{
    // cheap load first
    const std::size_t currentHead = head_.load(std::memory_order_relaxed);

    // check if empty
    if (currentHead == cachedTail_)
    {
        // Expensive load only if empty
        cachedTail_ = tail_.load(std::memory_order_acquire);

        // verify if really empty
        if (currentHead == cachedTail_)
        {
            return QueueEvent{MessageTypeNone}; // Yup, empty
        }
    }

    // cheap modulus substitute. Only works if Capacity_ is power of 2.
    auto item = std::move(buffer_[currentHead & Mask_]);

    // free up the slot
    head_.store(currentHead + 1, std::memory_order_release);
    return item;
}

