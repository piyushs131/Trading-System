#include "Api.hpp"
#include <atomic>
#include <sstream>

Api::Api() : authenticated(false) {
    // Constructor: no connection established yet
}

Api::~Api() {
    // Ensure the socket is closed on destruction
    orderSocket.close();
}

bool Api::authenticate(const std::string& user, const std::string& password) {
    // Connect to the local order server (mock exchange)
    if (!orderSocket.connect(ORDER_SERVER_HOST, ORDER_SERVER_PORT)) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"auth_failed\",\"reason\":\"connect_error\"}" << std::endl;
        return false;
    }
    // Send a mock login request
    std::stringstream ss;
    ss << "LOGIN " << user << " " << password;
    std::string loginMsg = ss.str();
    if (!orderSocket.send(loginMsg)) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"auth_failed\",\"reason\":\"send_error\"}" << std::endl;
        return false;
    }
    // Receive authentication response
    std::string response;
    if (!orderSocket.receive(response)) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"auth_failed\",\"reason\":\"no_response\"}" << std::endl;
        return false;
    }
    if (response.rfind("LOGIN_OK", 0) == 0) {
        authenticated = true;
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"auth_success\",\"user\":\"" << user << "\"}" << std::endl;
        return true;
    } else {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"auth_failed\",\"response\":\"" << response << "\"}" << std::endl;
        return false;
    }
}

int Api::placeOrder(const Order& order) {
    if (!authenticated) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"order_error\",\"error\":\"not_authenticated\"}" << std::endl;
        return -1;
    }
    // Generate a unique order ID (thread-safe)
    static std::atomic<int> orderIdCounter{0};
    int newId = orderIdCounter.fetch_add(1) + 1;
    // Compose order message for the mock server
    std::stringstream ss;
    ss << "ORDER id=" << newId 
       << " " << order.side 
       << " " << order.symbol 
       << " " << order.quantity 
       << " " << order.price;
    std::string orderMsg = ss.str();
    if (!orderSocket.send(orderMsg)) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"order_error\",\"order_id\":" << newId << ",\"reason\":\"send_failed\"}" << std::endl;
        return -1;
    }
    std::string response;
    if (!orderSocket.receive(response)) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"order_error\",\"order_id\":" << newId << ",\"reason\":\"no_ack\"}" << std::endl;
        return -1;
    }
    // Verify acknowledgment from server
    if (response.rfind("ORDER_ACK", 0) == 0) {
        size_t idPos = response.find("id=");
        if (idPos != std::string::npos) {
            // Parse ID from response and confirm it matches
            size_t begin = idPos + 3;
            size_t end = response.find(" ", begin);
            std::string idStr = response.substr(begin, end - begin);
            try {
                int ackId = std::stoi(idStr);
                if (ackId != newId) {
                    std::lock_guard<std::mutex> lock(logMutex);
                    std::cout << "{\"event\":\"order_error\",\"order_id\":" << newId << ",\"reason\":\"ack_id_mismatch\"}" << std::endl;
                    return -1;
                }
            } catch (...) {
                // Parsing failed (not expected in normal operation)
            }
        }
        return newId;  // Order acknowledged successfully
    } else {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"order_error\",\"order_id\":" << newId << ",\"response\":\"" << response << "\"}" << std::endl;
        return -1;
    }
}

void Api::disconnect() {
    // Clean up the connection
    orderSocket.close();
}