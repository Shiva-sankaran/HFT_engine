#include "trade_engine.h"


TradeEngine::TradeEngine(int n_symbols, double threshold_pct, int window_ms, double speedup)
    :        
    n_symbols_(n_symbols),
    threshold_pct_(threshold_pct),
    window_ms_(window_ms),
    speedup_(speedup),
    running_(true),
    window_time_(window_ms){std::cout<<"Trade Engine Intialized"<<std::endl;}
    
void TradeEngine::run(const std::vector<TradeEvent>& tradeEvents){

    if (tradeEvents.empty()) return;
    start();

    auto base_time = std::chrono::steady_clock::now();  
    auto base_event = tradeEvents[0].timestamp;     
    
    
    for (const auto& trade : tradeEvents) {
        auto event_offset = trade.timestamp - base_event;
        std::this_thread::sleep_until(base_time + event_offset);
        queue_.push(trade);

    }

    stop();
    print_summary();


}

void TradeEngine::start(){
    std::cout<<"Starting Trade Engine" << std::endl;
    spawn_alert_threads();
}

void TradeEngine::stop(){
    running_ = false;
    for (int i = 0; i < n_symbols_; ++i) {
        TradeEvent poison;
        poison.isPoisonPill = true;
        queue_.push(poison);
    }
    stop_alert_threads();
    std::cout<<"Stopped trading engine" << std::endl;
}

void TradeEngine::print_summary(){
    std::cout << "\nSummary:\n";
    std::cout << "Total trades: " << globalStats_.total_trades << '\n';
    std::cout << "Total alerts: " << globalStats_.total_alerts << '\n';

    for (const auto& [symbol,stat] : symbolStats_) {
        std::cout << "  " << symbol << ": " << stat.total_alerts << " alerts\n";

    }
}
void TradeEngine::spawn_alert_threads(){
    for(int i=0;i<n_symbols_;i++){
        alert_threads_.emplace_back(std::thread([this]() {
        
        while (true) {
            TradeEvent trade = queue_.pop_blocking();
            if (trade.isPoisonPill) break;  // Exit this thread
            process_trade(trade);
        }
        }));
        std::cout << "Spawned aleart thread" << std::endl;

    }
    
}

void TradeEngine::stop_alert_threads(){
    for(auto& alert_thread: alert_threads_){
        alert_thread.join();
        std::cout << "Stopped aleart thread" << std::endl;
    }
    
}


void TradeEngine::process_trade(TradeEvent trade){
    if (trade.isPoisonPill) return;
    std::lock_guard symbolLock(symbolStatMutexs[trade.symbol]);

    auto& stats = symbolStats_[trade.symbol];

    while(stats.window.size() != 0){
        TradeEvent ltrade = stats.window[0];
        if(trade.timestamp - ltrade.timestamp > window_time_){
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

    stats.vwap_numerator += trade.price * trade.volume;
    stats.total_volume += trade.volume;
    double vwap = stats.vwap_numerator/stats.total_volume;
    std::cout << "SYMBOL: " << trade.symbol
                << " PRICE: " << trade.price
                << " VOL: " << trade.volume << "\n";
    std::cout << "--> VWAP: "<<vwap<<"  TOTAL VOL: "<<symbolStats_[trade.symbol].total_volume <<std::endl;

    float deviation = std::abs(100* (trade.price - vwap)/vwap);

    globalStats_.total_trades ++;
    if(deviation > threshold_pct_){
        stats.total_alerts++;
        globalStats_.total_alerts++;
        std::cout << fmt::format("ALERT: {} trade at ${:.2f} deviates {:.2f}% from VWAP (${:.2f})", trade.symbol,trade.price,deviation,vwap) << std::endl;
    }
}