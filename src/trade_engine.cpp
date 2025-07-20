#include "trade_engine.h"
#include <immintrin.h>
#include <memory>

TradeEngine::TradeEngine(int n_symbols, int n_workers, double threshold_pct, int window_ms, double speedup)
    :        
    n_symbols_(n_symbols),
    n_workers_(n_workers),
    threshold_pct_(threshold_pct),
    window_ms_(window_ms),
    speedup_(speedup),
    running_(true)
    {
        std::cout<<"Trade Engine Intialized"<<std::endl;
        latency_log_ << "SYMBOL,TIMESTAMP,LATENCY(ns)\n";
    
    }

std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> TradeEngine::getSymbolQueues() {
        return symbol_to_order_queue_;
}


void TradeEngine::init(const std::vector<std::string>& symbols) {

    std::cout<<"Intializing Trade Engine" << std::endl;
    for (const auto& sym : symbols) {
        auto queue = std::make_shared<LockFreeQueue<Order>>(orderBookQueueCapacity);
        auto tradeQueue = std::make_shared<LockFreeQueue<TradeEvent>>(tradeQueueCapacity);

        symbol_to_order_queue_[sym] = queue;

        auto processor = std::make_unique<OrderBookWorker>(
            sym,
            queue,
            tradeQueue
            // std::make_unique<MatchingEngine>()
        );
        OrderBookProcessors.emplace_back(std::move(processor));


        
        symbol_to_trade_queue_[sym] = tradeQueue;

        auto tradeProcessor = std::make_unique<TradeProcessor>(
            sym,
            tradeQueue,
            window_ms_,
            threshold_pct_
        );

        TradeProcessors.emplace_back(std::move(tradeProcessor));
    }
}


void TradeEngine::start(){
    std::cout<<"Starting Trade Engine" << std::endl;
    start_order_book_workers();
    start_trade_processor_workers();
}


void TradeEngine::start_order_book_workers(){
    for(auto& processor: OrderBookProcessors){
        orderWorkers.emplace_back(std::thread([&processor](){
            processor->start();
        }));
    }
}

void TradeEngine::start_trade_processor_workers(){
    for(auto& processor: TradeProcessors){
        tradeWorkers.emplace_back(std::thread([&processor](){
            processor->start();
        }));
    }
}


void TradeEngine::stop(){
    running_ = false;
    stop_order_book_workers();
    stop_trade_processor_workers();
    std::cout<<"Stopped trading engine" << std::endl;
}

void TradeEngine::stop_order_book_workers(){
    for(auto& processor: OrderBookProcessors){
        processor->stop();
    }

    for(auto& worker: orderWorkers){
        worker.join();
    }
}

void TradeEngine::stop_trade_processor_workers(){
    for(auto& processor: TradeProcessors){
        processor->stop();
    }

    for(auto& worker: tradeWorkers){
        worker.join();
    }
}

std::unordered_map<std::string, SymbolStats> TradeEngine::getAllSymbolStats(){
    std::unordered_map<std::string, SymbolStats> symbolStats;

    for(auto& tp: TradeProcessors){
        symbolStats[tp->return_symbol()] = tp->return_stats();
    }

    return symbolStats;

}

void TradeEngine::print_summary() {
    auto stats = getAllSymbolStats();

    // Column headers
    std::cout << std::left << std::setw(10) << "Symbol"
              << std::right << std::setw(12) << "Trades"
              << std::setw(12) << "Volume"
              << std::setw(16) << "VWAP"
              << std::setw(16) << "Proc Latency (ns)"
              << std::setw(16) << "Wait Latency (ns)"
              << std::setw(10) << "Alerts"
              << std::endl;

    std::cout << std::string(92, '-') << std::endl;

    // Accumulators for averages
    long long total_trades = 0;
    long long total_volume = 0;
    long double total_vwap_numerator = 0;
    long long total_latency = 0;
    long long total_wait_latency = 0;
    long long total_alerts = 0;

    for (const auto& [symbol, stat] : stats) {
        long double vwap = stat.total_volume > 0 ? stat.vwap_numerator / stat.total_volume : 0;

        std::cout << std::left << std::setw(10) << symbol
                  << std::right << std::setw(12) << stat.total_trades
                  << std::setw(12) << stat.total_volume
                  << std::setw(16) << std::fixed << std::setprecision(4) << vwap
                  << std::setw(16) << (stat.total_trades > 0 ? stat.total_latency / stat.total_trades : 0)
                  << std::setw(16) << (stat.total_trades > 0 ? stat.wait_latency / stat.total_trades : 0)
                  << std::setw(10) << stat.total_alerts
                  << std::endl;

        // Aggregate
        total_trades += stat.total_trades;
        total_volume += stat.total_volume;
        total_vwap_numerator += stat.vwap_numerator;
        total_latency += stat.total_latency;
        total_wait_latency += stat.wait_latency;
        total_alerts += stat.total_alerts;
    }

    std::cout << std::string(92, '=') << std::endl;

    std::cout << std::left << std::setw(10) << "AVERAGE"
              << std::right << std::setw(12) << total_trades
              << std::setw(12) << total_volume
              << std::setw(16) << std::fixed << std::setprecision(4)
              << (total_volume > 0 ? total_vwap_numerator / total_volume : 0)
              << std::setw(16)
              << (total_trades > 0 ? total_latency / total_trades : 0)
              << std::setw(16)
              << (total_trades > 0 ? total_wait_latency / total_trades : 0)
              << std::setw(10) << total_alerts
              << std::endl;
}


// void TradeEngine::print_summary(){
//     std::cout << "\nSummary:\n";
//     std::cout << "Total trades: " << globalStats_.total_trades << '\n';
//     std::cout << "Total alerts: " << globalStats_.total_alerts << '\n';
//     std::cout << "Total Latency: " << globalStats_.total_latency/globalStats_.total_trades << "ns\n";
//     std::cout << "Wait Latency: " << globalStats_.wait_latency/globalStats_.total_trades << "ns\n";
//     std::cout << "Processing Latency: " << globalStats_.processing_latency/globalStats_.total_trades << "ns\n";
//     for (const auto& [symbol,stat] : symbolStats_) {
//         std::cout << "  " << symbol << ": " << stat.total_alerts << " alerts\n";

//     }
//     std::cout << "Average latency per symbol" << std::endl;
//     for (const auto& [symbol,stat] : symbolStats_) {
//         std::cout<< "  " << symbol << ": " << stat.total_latency/stat.total_trades << "ns\n";

//     }


//     std::cout <<"Total Latency Histogram Bins" <<std::endl;
//     {
//         std::lock_guard<std::mutex> lock(globalStats_.latency_mutex);
//         std::sort(globalStats_.latency_history.begin(), globalStats_.latency_history.end());
//         if (!globalStats_.latency_history.empty()) {
//             size_t n = globalStats_.latency_history.size();
//             std::cout << "P50 latency: " << globalStats_.latency_history[n / 2] << "ns\n";
//             std::cout << "P90 latency: " << globalStats_.latency_history[(n * 90) / 100] << "ns\n";
//             std::cout << "P99 latency: " << globalStats_.latency_history[(n * 99) / 100] << "ns\n";
//         }
//     }

//     std::cout << "Symbol to worker thread mapping" << std::endl;


// }




// void TradeEngine::spawn_workers(){
//     for(int i=0;i<n_workers_;i++){
//         std::cout << "Spawing worker: " << i <<std::endl;
//         workers.emplace_back(std::thread([i,this]() {
        
//         // unsigned int n_cores = std::thread::hardware_concurrency();
//         // int core_id = i % n_cores;
//         // cpu_set_t cpuset;
//         // CPU_ZERO(&cpuset);
//         // CPU_SET(core_id, &cpuset);  // pin thread i to core i
//         // pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

//         while (true) {
//             TradeEvent trade;
//             while(!workerDataQueues[i]->deque(trade)){
//             _mm_pause();
//             }

//             // TradeEvent trade = workerDataQueues[i]->pop_blocking();
//             if (trade.isPoisonPill) break;  // Exit this thread
//             process_trade(trade);
//         }
//         }));
//         std::cout << "Spawned worker: "<< i << std::endl;

//     }
    
// }


// void TradeEngine::stop_dispatcher(){
//     dispatcher.join();
//     std::cout << "Stopped dispathcer thread " << std::endl;
    
// }




// void TradeEngine::process_trade(TradeEvent trade){
//     if (trade.isPoisonPill) return;

//     globalStats_.wait_latency +=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - trade.received_time).count();
//     std::chrono::steady_clock::time_point processStartTime = std::chrono::steady_clock::now();
//     std::string symbol = trade.symbol;
  
//     auto& stats = symbolStats_[symbol];


//     while(stats.window.size() != 0){
//         TradeEvent ltrade = stats.window[0];
//         if(trade.timestamp - ltrade.timestamp > window_time_){
//             stats.window.pop_front();
            
//             stats.total_volume -= ltrade.volume;
//             stats.vwap_numerator -= ltrade.price * ltrade.volume;
//         }
//         else {
//             break;
//         }
//     }
//     stats.window.push_back(trade);

//     stats.total_trades ++;

//     stats.vwap_numerator += trade.price * trade.volume;
//     stats.total_volume += trade.volume;
//     double vwap = stats.vwap_numerator/stats.total_volume;

//     float deviation = std::abs(100* (trade.price - vwap)/vwap);

//     globalStats_.total_trades ++;
//     if(deviation > threshold_pct_){
//         stats.total_alerts++;
//         globalStats_.total_alerts++;
//         std::cout << fmt::format("ALERT: {} trade at ${:.2f} deviates {:.2f}% from VWAP (${:.2f})", symbol,trade.price,deviation,vwap) << std::endl;
//     }

//     globalStats_.processing_latency +=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - processStartTime).count();
    
//     auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - trade.received_time).count();
    
//     auto recent = globalStats_.get_recent_latencies(50);
//     if (!recent.empty()) {
//         std::sort(recent.begin(), recent.end());
//         size_t idx = static_cast<size_t>(std::ceil(recent.size() * 0.99)) - 1;
//         idx = std::min(idx, recent.size() - 1);  // clamp to max index

//         long long rolling_p99 = recent[idx];
//         if (latency > 2 * rolling_p99) {
//             std::cout << fmt::format("[WARN] Latency spike: {}ns > 2×P99 ({}ns)\n", latency, rolling_p99);
//         }
//     }
    
//     {
//         std::lock_guard<std::mutex> lock(globalStats_.latency_mutex);
//         if (globalStats_.latency_history.size() >= globalStats_.max_latency_samples) {
//             globalStats_.latency_history.pop_front();  // remove oldest
//         }
//         globalStats_.latency_history.push_back(latency);
//     }
    
//     globalStats_.total_latency += latency;
//     stats.total_latency += latency;

//     latency_log_ << symbol << "," << trade.timestamp.count() << "," << latency << "\n";
//     latency_log_.flush();

// }