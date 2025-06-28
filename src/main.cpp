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


int main() {
    
    auto DataQueue = std::make_shared<ThreadSafeQueue<TradeEvent>>();
    Client client("127.0.0.1", 5000, DataQueue);
    TradeEngine tradeEngine(4,4, 3.0,10000,DataQueue, 1.0 );
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
