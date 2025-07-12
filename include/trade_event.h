#pragma once
#include <chrono>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>


struct TradeEvent {
    int tradeID;
    std::chrono::microseconds timestamp;
    std::string symbol;  // now owns the memory
    double price;
    int volume;
    std::chrono::steady_clock::time_point received_time;
    bool isPoisonPill = false;

};
