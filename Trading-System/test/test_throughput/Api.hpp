#ifndef TEST_THROUGHPUT_API_HPP
#define TEST_THROUGHPUT_API_HPP

#include "utility.hpp"

/**
 * Api (throughput version) simulates a trading API without external I/O.
 * Authentication is immediate and order placement is processed in-memory.
 */
class Api {
public:
    Api();
    ~Api();
    bool authenticate(const std::string& user, const std::string& password);
    int placeOrder(const Order& order);
    void disconnect();
private:
    bool authenticated;
};

#endif // TEST_THROUGHPUT_API_HPP