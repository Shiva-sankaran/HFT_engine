#include <iostream>
#include <optional>
#include <chrono>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
struct TradeEvent {
    std::chrono::microseconds timestamp;
    std::string symbol;  // now owns the memory
    double price;
    int volume;
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
    while(getline(file,line)){
        trades.emplace_back(parse_line(line));
    }

    return trades;

}


int main() {

    auto start = std::chrono::steady_clock::now();
    auto trades = load_trades("trades.csv");
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Loaded " << trades.size() << " trades in "
            << elapsed.count() << " seconds\n";
}