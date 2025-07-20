#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

class Logger {
public:
    static void init();

    static std::shared_ptr<spdlog::logger> get(const std::string& name);

private:
    static void create_logger(const std::string& name, const std::string& file, const std::string& pattern);

    static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;
    static std::mutex mutex_;
};
