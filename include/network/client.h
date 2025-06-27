
#pragma once

#include<iostream>
#include<sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include "../thread_safe_queue.h"
#include "../trade_event.h"
class Client{
    private:
        std::string ip;
        int port;
        bool running;
        int sock;
        struct sockaddr_in server_addr;
        char buffer[3000];

    public:
        std::shared_ptr<ThreadSafeQueue<TradeEvent>> DataQueue;
        Client(std::string ip, int port, std::shared_ptr<ThreadSafeQueue<TradeEvent>> DataQueue);
        void start();
        void listen();
        void stop();
        void handle_line(const std::string& line);

};