
#pragma once

#include<iostream>
#include<sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <unordered_map>
#include <arpa/inet.h>
#include "../thread_safe_queue.h"
#include "../order.h"
#include "lock_free_queue.h"


class Client{
    private:
        std::string ip;
        int port;
        bool running;
        int sock;
        struct sockaddr_in server_addr;
        char buffer[3000];
        int n_workers_;
        std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> symbol_to_order_book_queue_;

    public:
        Client(std::string ip, int port, int n_workers_, std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> symbol_to_order_book_queue);
        void start();
        void listen();
        void stop();
        void handle_line(const std::string& line);
        Order parse_json(const std::string& line);

};