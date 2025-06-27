#include <stats.h>

std::vector<long long>  GlobalStats::get_recent_latencies(size_t count) {
    std::lock_guard<std::mutex> lock(latency_mutex);
    if (latency_history.size() < count) {
        return std::vector<long long>(latency_history.begin(), latency_history.end());
    }
    return std::vector<long long>(latency_history.end() - count, latency_history.end());
}
