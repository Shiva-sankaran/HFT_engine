#include "logger.h"
#include <iostream>

std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Logger::loggers_;
std::mutex Logger::mutex_;

void Logger::init() {
    spdlog::init_thread_pool(8192, 1);  // Queue size, num threads

    create_logger("orders", "logs/orders.log", "[%H:%M:%S.%e] [ORDERS] %v");
    create_logger("trades", "logs/trades.log", "[%H:%M:%S.%e] [TRADES] %v");
    create_logger("metrics", "logs/metrics.log", "[%H:%M:%S.%e] [METRICS] %v");
    std::cout << "CREATED LOGGERS" << std::endl;
}

void Logger::create_logger(const std::string& name, const std::string& file, const std::string& pattern) {
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file, true);
    auto logger = std::make_shared<spdlog::async_logger>(name, sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_pattern(pattern);
    logger->set_level(spdlog::level::debug);  // Can adjust per logger
    spdlog::register_logger(logger);

    std::lock_guard<std::mutex> lock(mutex_);
    loggers_[name] = logger;
}

// std::shared_ptr<spdlog::logger> Logger::get(const std::string& name) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     auto it = loggers_.find(name);
//     if (it != loggers_.end()) return it->second;
//     throw std::runtime_error("Logger not found: " + name);
// }

std::shared_ptr<spdlog::logger> Logger::get(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = loggers_.find(name);
    if (it != loggers_.end()) return it->second;

    std::cerr << "Logger not found: " << name << std::endl;
    std::abort();  // So you get a clean trace
}
