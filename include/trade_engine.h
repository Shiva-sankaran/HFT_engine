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
#include "order_book_worker.h"
#include "trade_processor.h"
#include "matching_engine.h"

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
        

        // std::vector<std::shared_ptr<LockFreeQueue<TradeEvent>>> workerDataQueues;
        // std::vector<std::shared_ptr<LockFreeQueue<Order>>> orderBookQueues;

        std::vector<std::unique_ptr<OrderBookWorker>> OrderBookProcessors;
        std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> symbol_to_order_queue_;
        std::vector<std::thread> orderWorkers;


        std::vector<std::unique_ptr<TradeProcessor>> TradeProcessors;
        std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<TradeEvent>>> symbol_to_trade_queue_;
        std::vector<std::thread> tradeWorkers;

        

        std::ofstream latency_log_{"latency_log.csv"};
        const size_t tradeQueueCapacity = 1024;
        const size_t orderBookQueueCapacity = 8192;

        void start_order_book_workers();
        void start_trade_processor_workers();
        void stop_order_book_workers();
        void stop_trade_processor_workers();


        // void spawn_dispatcher();
        // void spawn_workers();
        // void stop_dispatcher();
        // void stop_workers();
        


    public:
        TradeEngine(int n_symbols,
                    int n_workers,  
                    double threshold_pct, 
                    int window_ms,
                    double speedup = 1.0);
        void init(const std::vector<std::string>& symbols);
        void run(const std::vector<TradeEvent>& tradeEvents);
        void run(std::vector<TradeEvent>&& tradeEvents);
        void start();
        void stop();
        void print_summary();
        void process_trade(TradeEvent trade);
        std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> getSymbolQueues();


};

#endif