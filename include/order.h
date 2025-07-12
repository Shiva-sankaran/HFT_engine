#pragma once
#include <chrono>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>

enum class OrderType { NEW_LIMIT, PARTIAL_CANCEL, FULL_CANCEL, EXEC_VISIBLE, EXEC_INVISIBLE, HALT_RESUME };
enum class Side { BUY, SELL };


const std::unordered_map<int,OrderType> orderIntToTypeMap = {
        {1,OrderType::NEW_LIMIT},
        {2,OrderType::PARTIAL_CANCEL},
        {3,OrderType::FULL_CANCEL},
        {4,OrderType::EXEC_VISIBLE},
        {5,OrderType::EXEC_INVISIBLE},
        {7,OrderType::HALT_RESUME}
    };



struct Order {
    int orderId;
    std::chrono::microseconds timestamp;
    std::string symbol;  
    Side side;
    OrderType type;
    double price;
    int volume;
    std::chrono::steady_clock::time_point received_time;
    bool isPoisonPill = false;
    

};

Order parse_json(const std::string& line);
