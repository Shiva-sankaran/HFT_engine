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

struct TradeEvent {
    std::chrono::microseconds timestamp;
    std::string symbol;  // now owns the memory
    double price;
    int volume;
};

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

class ScopedTimer{

    private:
        std::string timerLabel;
        std::chrono::steady_clock::time_point startTime;

    public:
        ScopedTimer(std::string label){
            timerLabel = label;
            startTime = std::chrono::steady_clock::now();

        }
        ~ScopedTimer(){
            auto duration = std::chrono::steady_clock::now() - startTime;
            double seconds = std::chrono::duration<double>(duration).count();
            std::cout << fmt::format("[{}] Took {:.3f}s\n", timerLabel, seconds);

        }
};

TradeEvent parse_line(const std::string& line) {
    std::string_view view(line);
    size_t pos = 0;

    size_t next = view.find(',', pos);
    auto timestamp = std::chrono::microseconds(std::stoll(std::string(view.substr(pos, next - pos))));
    pos = next + 1;

    next = view.find(',', pos);
    std::string symbol(view.substr(pos, next - pos));  
    pos = next + 1;

    next = view.find(',', pos);
    double price = std::stod(std::string(view.substr(pos, next - pos)));
    pos = next + 1;

    int volume = std::stoi(std::string(view.substr(pos)));

    return {timestamp, std::move(symbol), price, volume};
}

std::vector<TradeEvent> load_trades(const std::string& filepath){
    std::ifstream file(filepath);
    std::string line;
    std::vector<TradeEvent> trades;
    trades.reserve(1'000'000);
    getline(file,line);
    while(getline(file,line)){
        trades.emplace_back(parse_line(line));
    }

    return trades;

}


void replay_with_stats(const std::vector<TradeEvent>& tradeEvents) {
    if (tradeEvents.empty()) return;

    auto base_time = std::chrono::steady_clock::now();  
    auto base_event = tradeEvents[0].timestamp;     
    std::unordered_map<std::string,SymbolStats> symbolStats;
    GlobalStats globalStats;

    std::chrono::microseconds window_time(5000);
    // std::deque<TradeEvent> window;
    
    for (const auto& trade : tradeEvents) {
        auto event_offset = trade.timestamp - base_event;
        std::this_thread::sleep_until(base_time + event_offset);

        auto& stats = symbolStats[trade.symbol];

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

    std::cout << "\nSummary:\n";
    std::cout << "Total trades: " << globalStats.total_trades << '\n';
    std::cout << "Total alerts: " << globalStats.total_alerts << '\n';

    for (const auto& [symbol,stat] : symbolStats) {
        std::cout << "  " << symbol << ": " << stat.total_alerts << " alerts\n";
}
}



int main() {

    auto start = std::chrono::steady_clock::now();
    auto trades = load_trades("trades.csv");
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Loaded " << trades.size() << " trades in "
            << elapsed.count() << " seconds\n";
        
    {
        ScopedTimer timer("Replay");
        replay_with_stats(trades);
    
    }
}
