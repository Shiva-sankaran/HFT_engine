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
        int n_symbols_;
        double threshold_pct_;
        int window_ms_;
        double speedup_;
        std::atomic<bool> running_;
        ThreadSafeQueue<TradeEvent> queue_;
        std::unordered_map<std::string,SymbolStats> symbolStats_;
        GlobalStats globalStats_;
        std::chrono::microseconds window_time_;

        std::vector<std::thread> alert_threads_;
        std::unordered_map<std::string,std::mutex> symbolStatMutexs;

        std::mutex queue_mutex_;
        std::condition_variable cond_;
        bool shutdown_ = false;

        

        void spawn_alert_threads();
        void stop_alert_threads();
        void process_trade(TradeEvent trade);
        


    public:
        TradeEngine(int n_symbols, double threshold_pct, int window_ms, double speedup = 1.0);
        void run(const std::vector<TradeEvent>& tradeEvents);
        void run(std::vector<TradeEvent>&& tradeEvents);
        void start();
        void stop();
        void print_summary();

};

#endif