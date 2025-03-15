#include "Api.hpp"
#include <atomic>
#include <iostream>

Api::Api() : authenticated(false) {}

Api::~Api() {}

bool Api::authenticate(const std::string& user, const std::string& password) {
    // Instantly authenticate (mock)
    authenticated = true;
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << "{\"event\":\"auth_success\",\"user\":\"" << user << "\"}" << std::endl;
    return true;
}

int Api::placeOrder(const Order& order) {
    if (!authenticated) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"order_error\",\"error\":\"not_authenticated\"}" << std::endl;
        return -1;
    }
    static std::atomic<int> orderIdCounter{0};
    int newId = orderIdCounter.fetch_add(1) + 1;
    // Simulate internal order processing (no network, minimal overhead)
    // (In a real system, this would update internal order books, etc.)
    return newId;
}

void Api::disconnect() {
    // No external resources to release in this mock
}