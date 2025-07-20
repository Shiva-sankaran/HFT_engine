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
    tradeQueue(tradeQueue),
    orderBook_(symbol)
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

    Logger::get("orders")->info("[Order Received] ID={} | Symbol={} | Side={} | Type={} | Price={} | Volume={} | Timestamp={}us | PoisonPill={}",
    order.orderId,
    order.symbol,
    (order.side == Side::BUY ? "BUY" : "SELL"),
    static_cast<int>(order.type) + 1,
    order.price,
    order.volume,
    order.timestamp.count(),
    order.isPoisonPill);


    std::optional<TradeEvent> maybeTrade = orderBook_.handleOrder(order);
    
    if (maybeTrade.has_value()) {

        Logger::get("trades")->info("[Trade Queued] TradeID={} | Price={} | Volume={}",
    maybeTrade->tradeID,
    maybeTrade->price,
    maybeTrade->volume);
    maybeTrade->created_timestamp = std::chrono::steady_clock::now();
    tradeQueue->enque(maybeTrade.value());
        
    }

}