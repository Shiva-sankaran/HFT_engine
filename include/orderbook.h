#pragma once

#include <map>
#include <queue>
#include <order.h>
#include <limits>
#include <iomanip>
#include <iostream>
#include <optional>

#include<trade_event.h>
#include<logger.h>
class OrderBook
{
private:
    /* data */
    std::string symbol;
    std::map<double,std::deque<Order>,std::greater<>> bids_;
    std::map<double,std::deque<Order>> asks_;
    std::unordered_map<int,Order*> order_lookup_;
    int tradeIdCounter_ = 1;

public:
    OrderBook(std::string symbol);
    void enter(const Order& order);
    bool cancel(int orderId, int volume);
    std::optional<TradeEvent> execOrderId(int orderId, int volume, double pric, Side side);
    std::optional<TradeEvent> handleOrder(Order& order);

    std::string generateTradeId();
    TradeEvent generateTrade(std::string symbol, int volume, double price, Side side);
    // int execBidAtPrice(double price, int volume);
    // int execAskAtPrice(double price, int volume);
    
};

