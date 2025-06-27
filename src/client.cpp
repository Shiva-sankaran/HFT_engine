#include "network/client.h"

Client::Client(std::string ip, int port , std::shared_ptr<ThreadSafeQueue<TradeEvent>> DataQueue)
    :
    ip(std::move(ip)),
    port(port),
    running(true),
    DataQueue(DataQueue){
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
            std::cout << "Received: " << line << std::endl;
            handle_line(line);
            partial.erase(0, pos + 1);
        }
    }
}

void Client::stop(){
    running = false;
    close(sock);
}

void Client::handle_line(const std::string& line){
    TradeEvent trade = parse_json(line);
    trade.received_time = std::chrono::steady_clock::now();
    DataQueue->push(trade);
}