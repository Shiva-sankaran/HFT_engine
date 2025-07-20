#include <orderbook.h>


OrderBook::OrderBook(std::string symbol)
    :
    symbol(symbol)
    {
        
    }



std::optional<TradeEvent> OrderBook::handleOrder(Order& order){

    if(order.type == OrderType::NEW_LIMIT){
        enter(order);
    } else if (order.type == OrderType::PARTIAL_CANCEL )
    {
        cancel(order.orderId, order.volume);
    } else if (order.type == OrderType::FULL_CANCEL)
    {
        cancel(order.orderId, -1);
    }else if(order.type == OrderType::EXEC_VISIBLE){
        return execOrderId(order.orderId, order.volume, order.price, order.side);
    } else if (order.type == OrderType::EXEC_INVISIBLE){
        //log
    } else {
        // halt
    }
    
    return std::nullopt;
}
void OrderBook::enter(const Order& order){

    // std::cout << "[ENTER] Order ID: " << order.orderId
    //           << " | Side: " << (order.side == Side::BUY ? "BUY" : "SELL")
    //           << " | Price: " << std::fixed << std::setprecision(2) << order.price
    //           << " | Volume: " << order.volume << std::endl;




    Logger::get("orders")->info("[ENTER] Order ID: {} | Symbol: {} | Side: {} | Price: {:.2f} | Volume: {}",
                            order.orderId,
                            order.symbol,
                            (order.side == Side::BUY ? "BUY" : "SELL"),
                            order.price,
                            order.volume);

    if (order.side == Side::BUY) {
        auto& queue = bids_[order.price];
        queue.push_back(order);  // makes an internal copy
        order_lookup_[order.orderId] = &queue.back();  // ✅ pointer to stored object
    } else {
        auto& queue = asks_[order.price];
        queue.push_back(order);
        order_lookup_[order.orderId] = &queue.back();  // ✅
    }

}


bool OrderBook::cancel(int order_id, int volume) {
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end()){
        Logger::get("orders")->warn("[CANCEL] Order ID={} not found", order_id);
        return false;
    }

    Order* order = it->second;  // ✅ fix: pointer access

    // Determine cancellation volume
    if (volume == -1 || volume >= order->volume) {
        // Full cancel
        volume = order->volume;
    }

    // std::cout << "[CANCEL] Order ID: " << order_id
    //           << " | Side: " << (order->side == Side::BUY ? "BUY" : "SELL")
    //           << " | Cancel Volume: " << volume
    //           << " | Original Volume: " << order->volume << std::endl;

    Logger::get("orders")->info("[CANCEL] Order ID: {} | Symbol: {} | Side: {} | Cancel Volume: {} | Original Volume: {}", 
                             order_id, 
                             order->symbol,
                             (order->side == Side::BUY ? "BUY" : "SELL"),
                             volume, 
                             order->volume);


    double price = order->price;
    if(order->side == Side::BUY){
        auto& book_side = bids_;
        auto level_it = book_side.find(price);
        if (level_it == book_side.end()) return false;

        auto& queue = level_it->second;
        for (auto q_it = queue.begin(); q_it != queue.end(); ++q_it) {
            if (q_it->orderId == order_id) {
                
                q_it->volume -= volume;
                
                if(q_it->volume == 0){
                    queue.erase(q_it);
                    if (queue.empty()) {
                        book_side.erase(level_it);
                    }
                    order_lookup_.erase(order_id);
                }
                    // Partial cancel
                   
            
                return true;
            }
        }
    } else {
        auto& book_side = asks_;
        auto level_it = book_side.find(price);
        if (level_it == book_side.end()) return false;

        auto& queue = level_it->second;
        for (auto q_it = queue.begin(); q_it != queue.end(); ++q_it) {
            if (q_it->orderId == order_id) {
                    // Partial cancel
                q_it->volume -= volume;
                
                if(q_it->volume == 0){
                    queue.erase(q_it);
                    if (queue.empty()) {
                        book_side.erase(level_it);
                    }
                    order_lookup_.erase(order_id);
                }

                return true;
            }
        }
    }

    

    return false;
}


std::optional<TradeEvent> OrderBook::execOrderId(int order_id, int volume, double price, Side side){
    if(order_lookup_.find(order_id) == order_lookup_.end()){
        return std::nullopt;
    }
    Order* resting_order = order_lookup_[order_id];
    resting_order->volume -= volume;

    // std::cout << "[EXEC] Order ID: " << order_id
    //           << " | Side: " << (side == Side::BUY ? "BUY" : "SELL")
    //           << " | Exec Volume: " << volume
    //           << " | Remaining: " << resting_order->volume << std::endl;

    Logger::get("orders")->info("[EXEC] Order ID: {} | Symbol: {} | Side: {} | Exec Volume: {} | Remaining: {}", 
                             order_id, 
                              resting_order->symbol,
                             (side == Side::BUY ? "BUY" : "SELL"),
                             volume, 
                             resting_order->volume);


    if (resting_order->volume <= 0) {
        if(resting_order->side == Side::BUY){
            auto& book_side = bids_;
            auto level_it = book_side.find(resting_order->price);
            if (level_it != book_side.end()) {
                auto& queue = level_it->second;
                for(auto it = queue.begin(); it!=queue.end(); it++){
                    if(it->orderId == order_id){
                        queue.erase(it);
                        // std::cout << "  -> Fully executed and removed\n";
                        break;
                    }
                }
                if (queue.empty()) {
                    book_side.erase(level_it);
                }
            }
        } else {
            auto& book_side = asks_;
            auto level_it = book_side.find(resting_order->price);
            if (level_it != book_side.end()) {
                auto& queue = level_it->second;
                for(auto it = queue.begin(); it!=queue.end(); it++){
                    if(it->orderId == order_id){
                        queue.erase(it);
                        // std::cout << "  -> Fully executed and removed\n";
                        break;
                    }
                }
                if (queue.empty()) {
                    book_side.erase(level_it);
                }
            }
        }
        order_lookup_.erase(order_id);
    }

    return generateTrade(symbol,volume,price,side);
}

std::string OrderBook::generateTradeId() {
        return symbol + std::to_string(tradeIdCounter_++);
}


TradeEvent OrderBook::generateTrade(std::string symbol, int volume, double price, Side side){

    return TradeEvent(generateTradeId(),symbol,price,volume,side);

}


// int OrderBook::execAskAtPrice(double price, int volume){

//     int executed_vol = 0;
//     while(volume > 0 && !asks_.empty()){

//         auto lowest_ask = asks_.begin()->first;
        
//         if(lowest_ask > price){
//             break;
//         }

//         auto& lowest_ask_queue = asks_.begin()->second;

//         while(!lowest_ask_queue.empty() && volume > 0){

//             auto& lowest_ask_order = lowest_ask_queue.front();
//             int fill_vol = std::min(lowest_ask_order.volume,volume);
//             executed_vol += fill_vol;
//             volume -= fill_vol;
//             lowest_ask_order.volume -= fill_vol;

//             if(lowest_ask_order.volume == 0){
//                 lowest_ask_queue.pop_front();
//             }   
            
//         }

//         if(lowest_ask_queue.empty()){
//             bids_.erase(bids_.begin());
//         }
//     }

//     return executed_vol;

// }


// int OrderBook::execBidAtPrice(double price, int volume){

//     int executed_vol = 0;
//     while(volume > 0 && !bids_.empty()){

//         auto highest_bid = bids_.begin()->first;
        
//         if(highest_bid < price){
//             break;
//         }

//         auto& highest_bid_queue = bids_.begin()->second;

//         while(!highest_bid_queue.empty() && volume > 0){

//             auto& highest_bid_order = highest_bid_queue.front();
//             int fill_vol = std::min(highest_bid_order.volume,volume);
//             volume -= fill_vol;
//             highest_bid_order.volume -= fill_vol;
//             executed_vol += fill_vol;


//             if(highest_bid_order.volume == 0){
//                 highest_bid_queue.pop_front();
//             }   
            
//         }

//         if(highest_bid_queue.empty()){
//             asks_.erase(asks_.begin());
//         }
//     }

//     return executed_vol;

// }
