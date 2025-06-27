#include "trade_engine.h"


TradeEngine::TradeEngine(int n_symbols, double threshold_pct, int window_ms, std::shared_ptr<ThreadSafeQueue<TradeEvent>> DataQueue, double speedup)
    :        
    n_symbols_(n_symbols),
    threshold_pct_(threshold_pct),
    window_ms_(window_ms),
    DataQueue(DataQueue),
    speedup_(speedup),
    running_(true),
    window_time_(window_ms){std::cout<<"Trade Engine Intialized"<<std::endl;}
    

void TradeEngine::start(){
    std::cout<<"Starting Trade Engine" << std::endl;
    spawn_alert_threads();
}

void TradeEngine::stop(){
    running_ = false;
    for (int i = 0; i < n_symbols_; ++i) {
        TradeEvent poison;
        poison.isPoisonPill = true;
        DataQueue->push(poison);
    }
    stop_alert_threads();
    std::cout<<"Stopped trading engine" << std::endl;
}

void TradeEngine::print_summary(){
    std::cout << "\nSummary:\n";
    std::cout << "Total trades: " << globalStats_.total_trades << '\n';
    std::cout << "Total alerts: " << globalStats_.total_alerts << '\n';
    std::cout << "Total Latency: " << globalStats_.total_latency/globalStats_.total_trades << "µs\n";
    std::cout << "Wait Latency: " << globalStats_.wait_latency/globalStats_.total_trades << "µs\n";
    std::cout << "Processing Latency: " << globalStats_.processing_latency/globalStats_.total_trades << "µs\n";
    for (const auto& [symbol,stat] : symbolStats_) {
        std::cout << "  " << symbol << ": " << stat.total_alerts << " alerts\n";

    }

    std::cout <<"Total Latency Histogram Bins" <<std::endl;
    {
        std::lock_guard<std::mutex> lock(globalStats_.latency_mutex);
        std::sort(globalStats_.latency_history.begin(), globalStats_.latency_history.end());
        if (!globalStats_.latency_history.empty()) {
            size_t n = globalStats_.latency_history.size();
            std::cout << "P50 latency: " << globalStats_.latency_history[n / 2] << "µs\n";
            std::cout << "P90 latency: " << globalStats_.latency_history[(n * 90) / 100] << "µs\n";
            std::cout << "P99 latency: " << globalStats_.latency_history[(n * 99) / 100] << "µs\n";
        }
    }

}
void TradeEngine::spawn_alert_threads(){
    for(int i=0;i<n_symbols_;i++){
        alert_threads_.emplace_back(std::thread([this]() {
        
        while (true) {
            TradeEvent trade = DataQueue->pop_blocking();
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

    globalStats_.wait_latency +=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - trade.received_time).count();
    std::chrono::steady_clock::time_point processStartTime = std::chrono::steady_clock::now();
    std::string symbol = trade.symbol;
    symbol.erase(std::remove_if(symbol.begin(), symbol.end(), ::isspace), symbol.end());
    {
        std::lock_guard<std::mutex> mapLock(globalMutexForSymbolMap_);
        if(symbolStatMutexs.find(symbol) == symbolStatMutexs.end()){
            symbolStatMutexs[symbol];
        }
    }
    std::lock_guard symbolLock(symbolStatMutexs[symbol]);

    auto& stats = symbolStats_[symbol];
    std::cout << "Window size for " << symbol << ": " << stats.window.size() << std::endl;

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
    std::cout << "SYMBOL: " << symbol
                << " PRICE: " << trade.price
                << " VOL: " << trade.volume << "\n";
    std::cout << "--> VWAP: "<<vwap<<"  TOTAL VOL: "<<symbolStats_[symbol].total_volume <<std::endl;

    float deviation = std::abs(100* (trade.price - vwap)/vwap);

    globalStats_.total_trades ++;
    if(deviation > threshold_pct_){
        stats.total_alerts++;
        globalStats_.total_alerts++;
        std::cout << fmt::format("ALERT: {} trade at ${:.2f} deviates {:.2f}% from VWAP (${:.2f})", symbol,trade.price,deviation,vwap) << std::endl;
    }

    globalStats_.processing_latency +=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - processStartTime).count();
    
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - trade.received_time).count();
    
    auto recent = globalStats_.get_recent_latencies(50);
    if (!recent.empty()) {
        std::sort(recent.begin(), recent.end());
        size_t idx = static_cast<size_t>(std::ceil(recent.size() * 0.99)) - 1;
        idx = std::min(idx, recent.size() - 1);  // clamp to max index

        long long rolling_p99 = recent[idx];
        if (latency > 2 * rolling_p99) {
            std::cout << fmt::format("[WARN] Latency spike: {}µs > 2×P99 ({}µs)\n", latency, rolling_p99);
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(globalStats_.latency_mutex);
        if (globalStats_.latency_history.size() >= globalStats_.max_latency_samples) {
            globalStats_.latency_history.pop_front();  // remove oldest
        }
        globalStats_.latency_history.push_back(latency);
    }
    
    globalStats_.total_latency += latency;


    

}