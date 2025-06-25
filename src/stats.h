#pragma once
#include "trade_event.h"
#include <deque>
struct GlobalStats {
    long long total_trades = 0;
    long long total_alerts = 0;

};

struct SymbolStats {
    std::deque<TradeEvent> window;
    long double vwap_numerator = 0;
    long long total_volume = 0;
    long long total_trades = 0;
    long long total_alerts = 0;

};