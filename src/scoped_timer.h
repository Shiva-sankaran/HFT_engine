#pragma once
#include <chrono>
#include <string>
#include <iostream>
#define FMT_HEADER_ONLY
#include <fmt/core.h>


class ScopedTimer{

    private:
        std::string timerLabel;
        std::chrono::steady_clock::time_point startTime;

    public:
        ScopedTimer(std::string label){
            timerLabel = label;
            startTime = std::chrono::steady_clock::now();

        }
        ~ScopedTimer(){
            auto duration = std::chrono::steady_clock::now() - startTime;
            double seconds = std::chrono::duration<double>(duration).count();
            std::cout << fmt::format("[{}] Took {:.3f}s\n", timerLabel, seconds);

        }
};