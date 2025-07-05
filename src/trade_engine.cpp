#include "trade_engine.h"
#include <immintrin.h>

TradeEngine::TradeEngine(int n_symbols, int n_workers, double threshold_pct, int window_ms, std::vector<std::shared_ptr<LockFreeQueue<TradeEvent>>> workerDataQueues, double speedup)
    :        
    n_symbols_(n_symbols),
    n_workers_(n_workers),
    threshold_pct_(threshold_pct),
    window_ms_(window_ms),
    speedup_(speedup),
    running_(true),
    window_time_(window_ms),
    workerDataQueues(workerDataQueues)
    {
        std::cout<<"Trade Engine Intialized"<<std::endl;
        latency_log_ << "SYMBOL,TIMESTAMP,LATENCY(ns)\n";
    
    }

void TradeEngine::start(){
    std::cout<<"Starting Trade Engine" << std::endl;
    std::cout << "Size of workerqueue: " << workerDataQueues.size() << std::endl;
    spawn_workers();
}

void TradeEngine::stop(){
    running_ = false;
    for (int i = 0; i < n_workers_; ++i) {
        TradeEvent poison;
        poison.isPoisonPill = true;
        while(!workerDataQueues[i]->enque(poison)){
            _mm_pause();
        }
        // workerDataQueues[i]->push(poison);
    }
    stop_workers();
    std::cout<<"Stopped trading engine" << std::endl;
}

void TradeEngine::print_summary(){
    std::cout << "\nSummary:\n";
    std::cout << "Total trades: " << globalStats_.total_trades << '\n';
    std::cout << "Total alerts: " << globalStats_.total_alerts << '\n';
    std::cout << "Total Latency: " << globalStats_.total_latency/globalStats_.total_trades << "ns\n";
    std::cout << "Wait Latency: " << globalStats_.wait_latency/globalStats_.total_trades << "ns\n";
    std::cout << "Processing Latency: " << globalStats_.processing_latency/globalStats_.total_trades << "ns\n";
    for (const auto& [symbol,stat] : symbolStats_) {
        std::cout << "  " << symbol << ": " << stat.total_alerts << " alerts\n";

    }
    std::cout << "Average latency per symbol" << std::endl;
    for (const auto& [symbol,stat] : symbolStats_) {
        std::cout<< "  " << symbol << ": " << stat.total_latency/stat.total_trades << "ns\n";

    }


    std::cout <<"Total Latency Histogram Bins" <<std::endl;
    {
        std::lock_guard<std::mutex> lock(globalStats_.latency_mutex);
        std::sort(globalStats_.latency_history.begin(), globalStats_.latency_history.end());
        if (!globalStats_.latency_history.empty()) {
            size_t n = globalStats_.latency_history.size();
            std::cout << "P50 latency: " << globalStats_.latency_history[n / 2] << "ns\n";
            std::cout << "P90 latency: " << globalStats_.latency_history[(n * 90) / 100] << "ns\n";
            std::cout << "P99 latency: " << globalStats_.latency_history[(n * 99) / 100] << "ns\n";
        }
    }

    std::cout << "Symbol to worker thread mapping" << std::endl;


}




void TradeEngine::spawn_workers(){
    for(int i=0;i<n_workers_;i++){
        std::cout << "Spawing worker: " << i <<std::endl;
        workers.emplace_back(std::thread([i,this]() {
        
        // unsigned int n_cores = std::thread::hardware_concurrency();
        // int core_id = i % n_cores;
        // cpu_set_t cpuset;
        // CPU_ZERO(&cpuset);
        // CPU_SET(core_id, &cpuset);  // pin thread i to core i
        // pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

        while (true) {
            TradeEvent trade;
            while(!workerDataQueues[i]->deque(trade)){
            _mm_pause();
            }

            // TradeEvent trade = workerDataQueues[i]->pop_blocking();
            if (trade.isPoisonPill) break;  // Exit this thread
            process_trade(trade);
        }
        }));
        std::cout << "Spawned worker: "<< i << std::endl;

    }
    
}


void TradeEngine::stop_dispatcher(){
    dispatcher.join();
    std::cout << "Stopped dispathcer thread " << std::endl;
    
}


void TradeEngine::stop_workers(){
    for (size_t i = 0; i < workers.size(); ++i) {
        workers[i].join();
        std::cout << "Stopped worker thread: " << i << std::endl;
    }
    
}

void TradeEngine::process_trade(TradeEvent trade){
    if (trade.isPoisonPill) return;

    globalStats_.wait_latency +=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - trade.received_time).count();
    std::chrono::steady_clock::time_point processStartTime = std::chrono::steady_clock::now();
    std::string symbol = trade.symbol;
  
    auto& stats = symbolStats_[symbol];


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

    float deviation = std::abs(100* (trade.price - vwap)/vwap);

    globalStats_.total_trades ++;
    if(deviation > threshold_pct_){
        stats.total_alerts++;
        globalStats_.total_alerts++;
        std::cout << fmt::format("ALERT: {} trade at ${:.2f} deviates {:.2f}% from VWAP (${:.2f})", symbol,trade.price,deviation,vwap) << std::endl;
    }

    globalStats_.processing_latency +=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - processStartTime).count();
    
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - trade.received_time).count();
    
    auto recent = globalStats_.get_recent_latencies(50);
    if (!recent.empty()) {
        std::sort(recent.begin(), recent.end());
        size_t idx = static_cast<size_t>(std::ceil(recent.size() * 0.99)) - 1;
        idx = std::min(idx, recent.size() - 1);  // clamp to max index

        long long rolling_p99 = recent[idx];
        if (latency > 2 * rolling_p99) {
            std::cout << fmt::format("[WARN] Latency spike: {}ns > 2×P99 ({}ns)\n", latency, rolling_p99);
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
    stats.total_latency += latency;

    latency_log_ << symbol << "," << trade.timestamp.count() << "," << latency << "\n";
    latency_log_.flush();

    

}