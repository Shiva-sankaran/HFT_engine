#pragma once
#include <chrono>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <order.h>

struct TradeEvent {
    std::string tradeID;
    std::string symbol; 
    double price;
    int volume;
    Side side;
    std::chrono::steady_clock::time_point created_timestamp;
    bool isPoisonPill = false;

    TradeEvent() 
    : tradeID(""), symbol(""), price(0.0), volume(0), side(Side::BUY),
      created_timestamp(std::chrono::steady_clock::now()), isPoisonPill(false) {}


    TradeEvent(std::string trade_id, std::string symbol, double price, int volume, Side side, bool isPoisonPill = false)
    :
    tradeID(trade_id),
    symbol(symbol),
    price(price),
    volume(volume),
    side(side),
    isPoisonPill(isPoisonPill)
    {
        
        // created_timestamp = std::chrono::steady_clock::now();

    }

};
