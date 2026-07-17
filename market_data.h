#pragma once
#include <array>
#include <string>


enum MessageType: char
{
    MessageTypeQuote =  'Q',
    MessageTypeTrade = 'T',
    MessageTypeOrder = 'O',

    MessageTypeNone = 'N', // Special; Queue empty flag
    MessageTypeTerm = 'E'  // Special; shutdown indicator flag
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
    std::array<char, 6>  sym;
    std::array<char, 19> timestampNs; // nanos since epoch
    std::array<char, 6>  bidQty;
    std::array<char, 6>  bidPrice;
    std::array<char, 6>  askQty;
    std::array<char, 6>  askPrice;
    std::array<char, 7>  seqNum; // monotonically increasing id
};

struct TradeDataWire
{
    std::array<char, 6>  sym;
    std::array<char, 19> timestampNs;
    std::array<char, 6>  qty;
    std::array<char, 6>  price;
};


struct OrderDataWire
{
    std::array<char, 6> sym;
    std::array<char, 19> timestampNs;
    std::array<char, 4> side;
    std::array<char, 6> qty;
    std::array<char, 6> price;
    std::array<char, 7>  corrId; // correlation ID -- seqNum of the quote that triggered the order
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


struct QuoteData
{
    // sym omitted in parsed data structure; invariant for a data feed
    //std::string sym;
    uint64_t timestampNs; // nanos since epoch
    int32_t bidQty;
    int32_t bidPrice;
    int32_t askQty;
    int32_t askPrice;
    int32_t seqNum;
};

struct TradeData
{
    // sym omitted in parsed data structure; invariant for a data feed
    //std::string sym;
    uint64_t timestampNs;
    int32_t qty;
    int32_t price;
};


// struct OrderData
// {
//     // sym omitted in parsed data structure; invariant for a data feed
//     //std::string sym;
//     uint64_t timestampNs;
//     int32_t qty;
//     int32_t price;
// };
