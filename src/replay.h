#pragma once
#include <vector>
#include "trade_event.h"
#include "stats.h"
#include <unordered_map>
#include <thread>
#include <iostream>
#define FMT_HEADER_ONLY

#include <fmt/core.h>  
#include "thread_safe_queue.h"
#include <atomic>

void process_trade(TradeEvent trade, std::unordered_map<std::string,SymbolStats>& symbolStats, GlobalStats& globalStats){

    if (trade.symbol.empty()) return;
    auto& stats = symbolStats[trade.symbol];
    std::chrono::microseconds window_time(5000);

    while(stats.window.size() != 0){
        TradeEvent ltrade = stats.window[0];
        if(trade.timestamp - ltrade.timestamp > window_time){
            stats.window.pop_front();
            
            stats.total_volume -= ltrade.volume;
            stats.vwap_numerator -= ltrade.price * ltrade.volume;
        }
        else {
            break;
        }
    }
    stats.window.push_back(trade);

    stats.total_trades ++;
    globalStats.total_trades ++;

    stats.vwap_numerator += trade.price * trade.volume;
    stats.total_volume += trade.volume;
    double vwap = stats.vwap_numerator/stats.total_volume;
    std::cout << "SYMBOL: " << trade.symbol
                << " PRICE: " << trade.price
                << " VOL: " << trade.volume << "\n";
    std::cout << "--> VWAP: "<<vwap<<"  TOTAL VOL: "<<symbolStats[trade.symbol].total_volume <<std::endl;

    float deviation = std::abs(100* (trade.price - vwap)/vwap);
    if(deviation > 3){
        stats.total_alerts++;
        globalStats.total_alerts++;
        std::cout << fmt::format("ALERT: {} trade at ${:.2f} deviates {:.2f}% from VWAP (${:.2f})", trade.symbol,trade.price,deviation,vwap) << std::endl;
    }

   
}
void replay_with_stats(const std::vector<TradeEvent>& tradeEvents) {
    if (tradeEvents.empty()) return;

    auto base_time = std::chrono::steady_clock::now();  
    auto base_event = tradeEvents[0].timestamp;     
    std::unordered_map<std::string,SymbolStats> symbolStats;
    GlobalStats globalStats;

    ThreadSafeQueue<TradeEvent> tradeQueue;

    std::atomic<bool> running = true;
    
    std::thread alert_thread([&]() {
        std::cout << "Spawned aleart thread" << std::endl;
        while (running || !tradeQueue.empty()){
            TradeEvent trade = tradeQueue.pop_blocking();
            process_trade(trade,symbolStats, globalStats);
        }

    });
    
    for (const auto& trade : tradeEvents) {
        auto event_offset = trade.timestamp - base_event;
        std::this_thread::sleep_until(base_time + event_offset);
        tradeQueue.push(trade);

    }

    running = false;
    tradeQueue.push({});
    alert_thread.join();

    std::cout << "\nSummary:\n";
    std::cout << "Total trades: " << globalStats.total_trades << '\n';
    std::cout << "Total alerts: " << globalStats.total_alerts << '\n';

    for (const auto& [symbol,stat] : symbolStats) {
        std::cout << "  " << symbol << ": " << stat.total_alerts << " alerts\n";

    }
}

