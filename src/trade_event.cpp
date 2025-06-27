#include "trade_event.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

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

    return {timestamp, std::move(symbol), price, volume,std::chrono::steady_clock::time_point{} };
}

TradeEvent parse_json(const std::string& line) {
    json j = json::parse(line);

    std::string symbol = j.at("symbol").get<std::string>();
    double price = j.at("price").get<double>();
    int volume = j.at("volume").get<int>();
    auto timestamp = std::chrono::microseconds(j.at("timestamp").get<int64_t>());

    return {timestamp, std::move(symbol), price, volume, std::chrono::steady_clock::time_point{}};
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
