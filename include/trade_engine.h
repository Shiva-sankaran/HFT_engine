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
#include "network/client.h"

class TradeEngine {

    private:
        int n_symbols_;
        int n_workers_;
        double threshold_pct_;
        int window_ms_;
        double speedup_;
        bool shutdown_ = false;
        std::atomic<bool> running_;

        std::chrono::microseconds window_time_;
        std::unordered_map<std::string,SymbolStats> symbolStats_;
        GlobalStats globalStats_;

        std::thread dispatcher;
        std::vector<std::thread> workers;
        

        std::vector<std::shared_ptr<ThreadSafeQueue<TradeEvent>>> workerDataQueues;

        

        std::ofstream latency_log_{"latency_log.csv"};

        void spawn_dispatcher();
        void spawn_workers();
        void stop_dispatcher();
        void stop_workers();
        


    public:
        TradeEngine(int n_symbols,int n_workers,  double threshold_pct, int window_ms,std::vector<std::shared_ptr<ThreadSafeQueue<TradeEvent>>> workerDataQueues, double speedup = 1.0);
        void run(const std::vector<TradeEvent>& tradeEvents);
        void run(std::vector<TradeEvent>&& tradeEvents);
        void start();
        void stop();
        void print_summary();
        void process_trade(TradeEvent trade);


};

#endif