#include <trade_processor.h>


TradeProcessor::TradeProcessor(std::string symbol, std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue, int window_time,double threshold_pct)
    :
    symbol(symbol),
    tradeQueue(tradeQueue),
    window_time_(window_time),
    threshold_pct_(threshold_pct)
{
}

TradeProcessor::~TradeProcessor()
{
}

void TradeProcessor::start(){

    std::cout << "Starting Trade Processor Worker for symbol: " << symbol << std::endl;
    while (true) {
        TradeEvent trade;
        while(!tradeQueue->deque(trade)){
        _mm_pause();
        }
        if (trade.isPoisonPill) break;  // Exit this thread

        auto log_msg = fmt::format(
            "[Trade Received] ID={} | Symbol={} | Side={} | Price={} | Volume={} | PoisonPill={}",
            trade.tradeID,
            trade.symbol,
            (trade.side == Side::BUY ? "BUY" : "SELL"),
            trade.price,
            trade.volume,
            trade.isPoisonPill
        );
        Logger::get("trades")->info(log_msg);

        process_trade(trade);
    }

}

void TradeProcessor::stop(){

    std::cout << "Stopping Trade Processor Worker for symbol: " << symbol << std::endl;
    TradeEvent poisonTrade;
    poisonTrade.isPoisonPill = true;

    tradeQueue->enque(poisonTrade);

}


void TradeProcessor::process_trade(TradeEvent& trade){

    if (trade.isPoisonPill) return;

    
    
    auto wait_latency = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - trade.created_timestamp).count();
    stats.wait_latency += wait_latency;


    auto process_start_time = std::chrono::steady_clock::now();

    update_stats(trade);
    check_alert(trade);


    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - process_start_time).count();
    stats.total_latency += latency;



}

void TradeProcessor::update_stats(const TradeEvent& trade) {
    // Lifetime stats
    stats.total_volume += trade.volume;
    stats.total_trades++;

    // Add new trade to window
    stats.window.push_back(trade);
    stats.vwap_numerator += trade.price * trade.volume;
    stats.running_volume += trade.volume;

    // Maintain sliding window
    while (!stats.window.empty()) {
        const TradeEvent& old_trade = stats.window.front();
        if (trade.created_timestamp - old_trade.created_timestamp > window_time_) {
            stats.vwap_numerator -= old_trade.price * old_trade.volume;
            stats.running_volume -= old_trade.volume;
            stats.window.pop_front();
        } else {
            break;
        }
    }
}

void TradeProcessor::check_alert(const TradeEvent& trade) {
    if (stats.running_volume == 0) return;  // avoid div by 0

    double vwap = stats.vwap_numerator / stats.running_volume;
    float deviation = std::abs(100.0 * (trade.price - vwap) / vwap);

    if (deviation > threshold_pct_) {
        Logger::get("metrics")->warn("ALERT: {} trade at ${:.2f} deviates {:.2f}% from VWAP (${:.2f})",
                                     trade.symbol, trade.price, deviation, vwap);
    }
}

SymbolStats TradeProcessor::return_stats(){
    return stats;
}

std::string TradeProcessor::return_symbol(){
    return symbol;
}
