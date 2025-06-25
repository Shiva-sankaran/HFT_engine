#include <iostream>
#include <optional>
#include <chrono>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <cmath>
#include <deque>
#define FMT_HEADER_ONLY
#include <fmt/core.h>  // Main header
#include <unordered_map>

#include "stats.h"
#include "scoped_timer.h"
#include "replay.h"
#include "trade_event.h"


int main() {

    auto start = std::chrono::steady_clock::now();
    auto trades = load_trades("../trades.csv");
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Loaded " << trades.size() << " trades in "
            << elapsed.count() << " seconds\n";
        
    {
        ScopedTimer timer("Replay");
        replay_with_stats(trades);
    
    }
}
