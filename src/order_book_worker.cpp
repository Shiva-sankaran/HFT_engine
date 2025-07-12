#include <order_book_worker.h>

#include <iostream>

OrderBookWorker::OrderBookWorker(
    std::string symbol,
    std::shared_ptr<LockFreeQueue<Order>> orderQueue, 
    std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue
    )
    :
    symbol(symbol),
    orderQueue(orderQueue),
    tradeQueue(tradeQueue)
    {

}

OrderBookWorker::~ OrderBookWorker()
{
}


void OrderBookWorker::start(){
    std::cout << "Starting Order Book Worker for symbol: " << symbol << std::endl;
    while (true) {
        Order order;
        while(!orderQueue->deque(order)){
        _mm_pause();
        }

        // TradeEvent trade = workerDataQueues[i]->pop_blocking();
        if (order.isPoisonPill) break;  // Exit this thread
        process_order(order);
    }
}

void OrderBookWorker::stop(){

    std::cout << "Stopping Order Book Worker for symbol: " << symbol << std::endl;
    Order poisonOrder;
    poisonOrder.isPoisonPill = true;
    orderQueue->enque(poisonOrder);

}

void OrderBookWorker::process_order(Order& order){

    std::cout << "[Order Received] "
          << "ID=" << order.orderId
          << ", Symbol=" << order.symbol
          << ", Side=" << (order.side == Side::BUY ? "BUY" : "SELL")
          << ", Type=" << static_cast<int>(order.type)
          << ", Price=" << order.price
          << ", Volume=" << order.volume
          << ", Timestamp=" << order.timestamp.count()
          << "us"
          << ", PoisonPill=" << std::boolalpha << order.isPoisonPill
          << std::endl;

}