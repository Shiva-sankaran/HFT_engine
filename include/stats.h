#pragma once
#include "trade_event.h"
#include <deque>
#include <atomic>
struct GlobalStats {
    std::atomic<long long> total_trades{0};
    std::atomic<long long> total_alerts{0};
    std::atomic<long long> total_latency{0};

};

struct SymbolStats {
    std::deque<TradeEvent> window;
    long double vwap_numerator = 0;
    long long total_volume = 0;
    long long total_trades = 0;
    long long total_alerts = 0;

};