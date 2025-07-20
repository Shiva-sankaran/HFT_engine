#pragma once

#include <string>
#include <memory>
#include <iostream>
#include <immintrin.h>
#define FMT_HEADER_ONLY
#include <fmt/core.h>  // Main header


#include <lock_free_queue.h>
#include <trade_event.h>
#include <stats.h>
#include <logger.h>

class TradeProcessor
{
private:
    std::string symbol;
    std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue;
    std::chrono::microseconds window_time_;
    double threshold_pct_;
    SymbolStats stats;




public:
    TradeProcessor(std::string symbol, std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue, int window_time, double threshold_pct);
    ~TradeProcessor();
    void start();
    void stop();
    void process_trade(TradeEvent& trade);
    SymbolStats return_stats();
    std::string return_symbol();

    void update_stats(const TradeEvent& Trade);
    void check_alert(const TradeEvent& trade);
};

