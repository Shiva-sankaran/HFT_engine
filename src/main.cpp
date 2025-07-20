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

    TradeEngine tradeEngine(n_symbols,n_workers, threshold_pct,window_ms, speed_up);
    tradeEngine.init(symbols);

    Client client(serverIPAddress,serverPort,n_workers, tradeEngine.getSymbolQueues());

    std::cout << "!!!!!!!!!!!!!!!!!";
    client.start();
    std::thread listenerThread([&client]() {
        std::cout<<"Listener Thread Started" << std::endl;
        client.listen();
    });

    std::thread engineThread([&tradeEngine]() {
        std::cout<<"Engine Thread Started" << std::endl;
        tradeEngine.start();
    });
    
    listenerThread.join();
    engineThread.join();
    tradeEngine.print_summary();
    client.stop();
    tradeEngine.stop();
    return 0;
}
