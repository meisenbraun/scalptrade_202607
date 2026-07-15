#pragma once
#include <array>


enum MessageType: char
{
    MessageTypeQuote =  'Q',
    MessageTypeTrade = 'T',
    MessageTypeOrder = 'O',
    MessageTypeNone = 'N'
};

enum OrderSide: char
{
    OrderSideBuy = 'B',
    OrderSideSell = 'S'
};

// Wire types
#pragma pack(push, 1)

struct WireHeader
{
    MessageType msgType;
};

struct QuoteDataWire
{
    std::array<char, 6> sym;
    std::array<char, 19> timestampNs; // nanos since epoch
    std::array<char, 6> bidQty;
    std::array<char, 6> bitPrice;
    std::array<char, 6> askQty;
    std::array<char, 6> askPrice;
};

struct TradeDataWire
{
    std::array<char, 6> sym;
    std::array<char, 19> timestampNs;
    std::array<char, 6> qty;
    std::array<char, 6> price;
};


struct OrderDataWire
{
    std::array<char, 6> sym;
    std::array<char, 19> timestampNs;
    std::array<char, 6> qty;
    std::array<char, 6> price;
};


// Would use a std::variant here, but
// Limited to C++14 and std::variant is not available
struct QueueEvent
{
    MessageType msgType;

    union
    {
        QuoteDataWire quote;
        TradeDataWire trade;
    };
};
#pragma pack(pop)