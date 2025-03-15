#ifndef TEST_LATENCY_UTILITY_HPP
#define TEST_LATENCY_UTILITY_HPP

#include <iostream>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

// Global mutex for thread-safe logging
inline std::mutex logMutex;

// High-resolution timestamp in nanoseconds
inline long long nowNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

// Order data structure
struct Order {
    int id;
    std::string symbol;
    std::string side;
    int quantity;
    double price;
    Order(int i, const std::string& sym, const std::string& s, int q, double p)
        : id(i), symbol(sym), side(s), quantity(q), price(p) {}
    Order() : id(0), symbol(""), side(""), quantity(0), price(0.0) {}
};

// Constants for local server ports
inline const char* ORDER_SERVER_HOST = "127.0.0.1";
inline const unsigned short ORDER_SERVER_PORT = 5555;
inline const unsigned short WEBSOCKET_PORT = 9001;

#endif 