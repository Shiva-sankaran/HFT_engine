#include <trade_processor.h>


TradeProcessor::TradeProcessor(std::string symbol, std::shared_ptr<LockFreeQueue<TradeEvent>> tradeQueue)
    :
    symbol(symbol),
    tradeQueue(tradeQueue)
{
}

TradeProcessor::~TradeProcessor()
{
}

void TradeProcessor::start(){

    std::cout << "Starting Trade Processor Worker for symbol: " << symbol << std::endl;
    while (true) {
        TradeEvent trade;
        while(!tradeQueue->deque(trade)){
        _mm_pause();
        }

        // TradeEvent trade = workerDataQueues[i]->pop_blocking();
        if (trade.isPoisonPill) break;  // Exit this thread
        // process_trade(trade);
    }

}

void TradeProcessor::stop(){

    std::cout << "Stopping Trade Processor Worker for symbol: " << symbol << std::endl;
    TradeEvent poisonTrade;
    poisonTrade.isPoisonPill = true;

    tradeQueue->enque(poisonTrade);

}


// void TradeProcessor::process_trade(TradeEvent& trade){

// }