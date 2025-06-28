#pragma once
#include "trade_event.h"
#include <deque>
#include <atomic>
#include <mutex>
struct GlobalStats {
    std::atomic<long long> total_trades{0};
    std::atomic<long long> total_alerts{0};
    std::atomic<long long> total_latency{0};
    std::atomic<long long> processing_latency{0};
    std::atomic<long long> wait_latency{0};

    std::deque<long long> latency_history;
    mutable std::mutex latency_mutex;
    int max_latency_samples = 10000;
    std::vector<long long> get_recent_latencies(size_t count);


};

struct SymbolStats {
    std::deque<TradeEvent> window;
    long double vwap_numerator = 0;
    long long total_volume = 0;
    long long total_trades = 0;
    long long total_alerts = 0;

    long long total_latency = 0;
    long long total_lock_latency = 0;



};


