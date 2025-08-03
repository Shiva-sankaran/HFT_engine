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

#include "stats.h"
#include "util/scoped_timer.h"
#include "replay.h"
#include "trade_event.h"
#include "trade_engine.h"
#include "network/client.h"
#include "lock_free_queue.h"
#include "network/feed_handler.h"

#include<logger.h>



int main() {

    Logger::init(); 

    int n_symbols = 4;
    double threshold_pct = 3.0;
    int window_ms = 10000;
    double speed_up = 1.0;
    std::string serverIPAddress = "127.0.0.1";
    int serverPort = 5000;
    int n_workers = 4;
    
    const std::vector<std::string>& symbols = {"AAPL","MSFT","GOOG","AMZN","INTC"};

    int n_feed_threads = 2;
    int n_sockets_per_feed_thread = 2;
    std::vector<MarketFeed> feeds;
    MarketFeed feed1{"NASDAQ-AAPL", 5000, "127.0.0.1"};
    MarketFeed feed2{"NASDAQ-MSFT", 5001, "127.0.0.1"};
    MarketFeed feed3{"NASDAQ-GOOG", 5002, "127.0.0.1"};
    MarketFeed feed4("NYSE-AMZN", 5003, "127.0.0.1");
    feeds.emplace_back(feed1);
    feeds.emplace_back(feed2);
    feeds.emplace_back(feed3);
    feeds.emplace_back(feed4);


    TradeEngine tradeEngine(n_symbols,n_workers, threshold_pct,window_ms, speed_up);
    tradeEngine.init(symbols);

    FeedHandler feedHandler(n_feed_threads,n_sockets_per_feed_thread,feeds,tradeEngine.getSymbolQueues());
    feedHandler.start();
    std::thread feedThread([&feedHandler](){
        std::cout<<"Market Feed Listener Thread Started" << std::endl;
        feedHandler.listen();
    });

    feedThread.join();
    
    feedHandler.stop();
    
    // Client client(serverIPAddress,serverPort,n_workers, tradeEngine.getSymbolQueues());

    // std::cout << "!!!!!!!!!!!!!!!!!";
    // client.start();
    // std::thread listenerThread([&client]() {
    //     std::cout<<"Listener Thread Started" << std::endl;
    //     client.listen();
    // });

    // std::thread engineThread([&tradeEngine]() {
    //     std::cout<<"Engine Thread Started" << std::endl;
    //     tradeEngine.start();
    // });
    
    // listenerThread.join();
    // engineThread.join();
    // tradeEngine.print_summary();
    // client.stop();
    // tradeEngine.stop();

    

    return 0;
}
