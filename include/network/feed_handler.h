
#pragma once

#include<iostream>
#include<sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <unordered_map>
#include <arpa/inet.h>
#include <thread>
#include <sys/epoll.h>
#include <fcntl.h>

#include "../thread_safe_queue.h"
#include "../order.h"
#include "lock_free_queue.h"
#include "market_feed.h"
class FeedHandler{
    private:
        int _n_threads;
        int _n_sockets_per_thread;
        std::vector<MarketFeed> _feeds;
        // std::string _ip;
        // int _base_port;
        bool running;
        std::vector<std::thread> _epoll_workers;
        std::vector<int> _epoll_fds;
        
        // int sock;
        // struct sockaddr_in server_addr;
        // char buffer[3000];
        // int n_workers_;
        std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> _symbol_to_order_book_queue_;

    public:
        FeedHandler(int n_threads, int n_sockets_per_thread, std::vector<MarketFeed> feeds, std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> symbol_to_order_book_queue);
        void start();
        void listen();
        void stop();
        void handle_line(const std::string& line);
        Order parse_json(const std::string& line);

        int create_feed_connection(MarketFeed feed);

};