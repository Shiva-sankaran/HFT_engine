#include "network/client.h"
#include <immintrin.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

Client::Client(std::string ip, int port , int n_workers_, std::unordered_map<std::string, std::shared_ptr<LockFreeQueue<Order>>> symbol_to_order_book_queue)
    :
    ip(std::move(ip)),
    port(port),
    running(true),
    n_workers_(n_workers_),
    symbol_to_order_book_queue_(symbol_to_order_book_queue){
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

    }

void Client::start(){

    

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);


    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to server at " << ip << ":" << port << "\n";

}

void Client::listen(){

    std::string partial;
    while (running) {
        ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) {
            std::cout << "Disconnected or error.\n";
            break;
        }

        buffer[bytes] = '\0';
        partial += buffer;

        size_t pos;
        while ((pos = partial.find('\n')) != std::string::npos) {
            std::string line = partial.substr(0, pos);
            // std::cout << "Received: " << line << std::endl;
            handle_line(line);
            partial.erase(0, pos + 1);
        }
    }
}

void Client::stop(){
    running = false;
    close(sock);
}

Order Client::parse_json(const std::string& line) {
    json j = json::parse(line);
    std::string symbol = j.at("symbol").get<std::string>();
    double price = j.at("price").get<double>();
    int volume = j.at("size").get<int>();
    auto timestamp = std::chrono::microseconds(j.at("timestamp").get<int64_t>());
    OrderType orderType = orderIntToTypeMap.at(j.at("type").get<int>());
    int direction = j.at("direction").get<int>();
    int orderID = j.at("order_id").get<int>();

    Side direction_enum = (direction == 1) ? Side::BUY : Side::SELL;
    return Order{orderID, timestamp, std::move(symbol), direction_enum, orderType, price, volume, std::chrono::steady_clock::time_point{}};
}

void Client::handle_line(const std::string& line){
    Order order = parse_json(line);
    
    order.received_time = std::chrono::steady_clock::now();
    while(!symbol_to_order_book_queue_[order.symbol]->enque(order)){
        _mm_pause();
    }

}