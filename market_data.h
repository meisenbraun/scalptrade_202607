#pragma once
#include <array>


enum MessageType: char
{
    MessageTypeQuote =  'Q',
    MessageTypeTrade = 'T',
    MessageTypeOrder = 'O'
};

enum OrderSide: char
{
    OrderSideBuy = 'B',
    OrderSideSell = 'S'
};

// Wire types
#pragma pack(1)
struct QuoteData
{
    MessageType msgType;
    std::array<char, 6> sym;
    std::array<char, 19> timestampNs; // nanos since epoch
    std::array<char, 6> bidQty;
    std::array<char, 6> bitPrice;
    std::array<char, 6> askQty;
    std::array<char, 6> askPrice;
};

struct TradeData
{
    MessageType msgType;
    std::array<char, 6> sym;
    std::array<char, 19> timestampNs;
    std::array<char, 6> qty;
    std::array<char, 6> price;
};


struct OrderData
{
    MessageType msgType;
    std::array<char, 6> sym;
    std::array<char, 19> timestampNs;
    std::array<char, 6> qty;
    std::array<char, 6> price;
};
