#pragma once

#ifndef TRADE_ENGINE_H
#define TRADE_ENGINE_H


#include <atomic>
#include <vector>
#include <thread>
#include <unordered_map>
#define FMT_HEADER_ONLY
#include <fmt/core.h>  // Main header
#include <iostream>
#include <chrono>

#include "thread_safe_queue.h"
#include "trade_event.h"
#include "replay.h"
#include "stats.h"

class TradeEngine {

    private:
        double threshold_pct_;
        int window_ms_;
        double speedup_;
        std::atomic<bool> running_;
        ThreadSafeQueue<TradeEvent> queue_;
        std::unordered_map<std::string,SymbolStats> symbolStats_;
        GlobalStats globalStats_;
        std::thread alert_thread_;
        std::chrono::microseconds window_time_;
        

        void spawn_alert_thread();
        void process_trade(TradeEvent trade);


    public:
        TradeEngine(double threshold_pct, int window_ms, double speedup = 1.0);
        void run(const std::vector<TradeEvent>& tradeEvents);
        void run(std::vector<TradeEvent>&& tradeEvents);

};

#endif