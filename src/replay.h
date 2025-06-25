// #pragma once
// #include <vector>
// #include "trade_event.h"
// #include "stats.h"
// #include <unordered_map>
// #include <thread>
// #include <iostream>
// #define FMT_HEADER_ONLY

// #include <fmt/core.h>  
// #include "thread_safe_queue.h"
// #include <atomic>


// // void replay_with_stats(const std::vector<TradeEvent>& tradeEvents) {
// //     if (tradeEvents.empty()) return;

// //     auto base_time = std::chrono::steady_clock::now();  
// //     auto base_event = tradeEvents[0].timestamp;     
// //     std::unordered_map<std::string,SymbolStats> symbolStats;
// //     GlobalStats globalStats;

// //     ThreadSafeQueue<TradeEvent> tradeQueue;

// //     std::atomic<bool> running = true;
    
// //     std::thread alert_thread([&]() {
// //         std::cout << "Spawned aleart thread" << std::endl;
// //         while (running || !tradeQueue.empty()){
// //             TradeEvent trade = tradeQueue.pop_blocking();
// //             process_trade(trade,symbolStats, globalStats);
// //         }

// //     });
    
// //     for (const auto& trade : tradeEvents) {
// //         auto event_offset = trade.timestamp - base_event;
// //         std::this_thread::sleep_until(base_time + event_offset);
// //         tradeQueue.push(trade);

// //     }

// //     running = false;
// //     tradeQueue.push({});
// //     alert_thread.join();

// //     std::cout << "\nSummary:\n";
// //     std::cout << "Total trades: " << globalStats.total_trades << '\n';
// //     std::cout << "Total alerts: " << globalStats.total_alerts << '\n';

// //     for (const auto& [symbol,stat] : symbolStats) {
// //         std::cout << "  " << symbol << ": " << stat.total_alerts << " alerts\n";

// //     }
// // }

