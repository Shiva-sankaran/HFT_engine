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
    std::chrono::steady_clock::time_point received_time;
    bool isPoisonPill = false;

};


TradeEvent parse_line(const std::string& line);
TradeEvent parse_json(const std::string& line);
std::vector<TradeEvent> load_trades(const std::string& filepath);
