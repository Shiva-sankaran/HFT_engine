
#pragma once
#include <order_book_worker.h>
#include <lock_free_queue.h>
#include <order.h>
#include <trade_event.h>


#include <memory>
#include <string>
#include <immintrin.h>


class OrderBookWorker
{
private:
    
    std::shared_ptr<LockFreeQueue<Order>> orderQueue;
    std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue;
public:
    std::string symbol;
    OrderBookWorker(std::string symbol,std::shared_ptr<LockFreeQueue<Order>> orderQueue, std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue);
    ~ OrderBookWorker();
    void start();
    void stop();
    void process_order(Order& order);
};

