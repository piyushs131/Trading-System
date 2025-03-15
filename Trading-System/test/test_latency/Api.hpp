#ifndef TEST_LATENCY_API_HPP
#define TEST_LATENCY_API_HPP

#include "utility.hpp"
#include "Socket.hpp"

/**
 * Api simulates a trading API (e.g., exchange REST API).
 * Provides authentication and order placement functions.
 */
class Api {
public:
    Api();
    ~Api();
    // Authenticate with given credentials (mocked)
    bool authenticate(const std::string& user, const std::string& password);
    // Place an order (returns order ID on success, -1 on failure)
    int placeOrder(const Order& order);
    // Disconnect API (closes connection)
    void disconnect();
private:
    bool authenticated;
    Socket orderSocket;  // Socket connection to mock order server
};

#endif // TEST_LATENCY_API_HPP