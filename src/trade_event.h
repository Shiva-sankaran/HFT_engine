#pragma once
#include <chrono>
#include <string>
#include <fstream>
#include <vector>

struct TradeEvent {
    std::chrono::microseconds timestamp;
    std::string symbol;  // now owns the memory
    double price;
    int volume;
    bool isPoisonPill = false;

    

};


TradeEvent parse_line(const std::string& line);
std::vector<TradeEvent> load_trades(const std::string& filepath);
