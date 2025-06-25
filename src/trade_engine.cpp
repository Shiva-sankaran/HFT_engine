#include "trade_engine.h"


TradeEngine::TradeEngine(double threshold_pct, int window_ms, double speedup)
    :        
    threshold_pct_(threshold_pct),
    window_ms_(window_ms),
    speedup_(speedup),
    running_(true),
    window_time_(window_ms){std::cout<<"Trade Engine Intialized"<<std::endl;}
    
void TradeEngine::run(const std::vector<TradeEvent>& tradeEvents){

    if (tradeEvents.empty()) return;
    auto base_time = std::chrono::steady_clock::now();  
    auto base_event = tradeEvents[0].timestamp;     
    
    spawn_alert_thread();
    
    
    for (const auto& trade : tradeEvents) {
        auto event_offset = trade.timestamp - base_event;
        std::this_thread::sleep_until(base_time + event_offset);
        queue_.push(trade);

    }

    running_ = false;
    queue_.push({});
    alert_thread_.join();

    std::cout << "\nSummary:\n";
    std::cout << "Total trades: " << globalStats_.total_trades << '\n';
    std::cout << "Total alerts: " << globalStats_.total_alerts << '\n';

    for (const auto& [symbol,stat] : symbolStats_) {
        std::cout << "  " << symbol << ": " << stat.total_alerts << " alerts\n";

    }

}

void TradeEngine::run(std::vector<TradeEvent>&& tradeEvents) {
    run(tradeEvents);  
}

void TradeEngine::spawn_alert_thread(){
    alert_thread_ = std::thread([this]() {
        std::cout << "Spawned aleart thread" << std::endl;
        while (running_ || !queue_.empty()){
            TradeEvent trade = queue_.pop_blocking();
            process_trade(trade);
        }

    });

}

void TradeEngine::process_trade(TradeEvent trade){
    if (trade.symbol.empty()) return;
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
    globalStats_.total_trades ++;

    stats.vwap_numerator += trade.price * trade.volume;
    stats.total_volume += trade.volume;
    double vwap = stats.vwap_numerator/stats.total_volume;
    std::cout << "SYMBOL: " << trade.symbol
                << " PRICE: " << trade.price
                << " VOL: " << trade.volume << "\n";
    std::cout << "--> VWAP: "<<vwap<<"  TOTAL VOL: "<<symbolStats_[trade.symbol].total_volume <<std::endl;

    float deviation = std::abs(100* (trade.price - vwap)/vwap);
    if(deviation > threshold_pct_){
        stats.total_alerts++;
        globalStats_.total_alerts++;
        std::cout << fmt::format("ALERT: {} trade at ${:.2f} deviates {:.2f}% from VWAP (${:.2f})", trade.symbol,trade.price,deviation,vwap) << std::endl;
    }


}