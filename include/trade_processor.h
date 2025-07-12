#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <immintrin.h>


#include <lock_free_queue.h>
#include <trade_event.h>

class TradeProcessor
{
private:
    std::string symbol;
    std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue;
public:
    TradeProcessor(std::string symbol, std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue);
    ~TradeProcessor();
    void start();
    void stop();
    void process_trade(TradeEvent& trade);
};

