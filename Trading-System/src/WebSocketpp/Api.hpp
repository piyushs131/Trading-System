#ifndef WEBSOCKETPP_API_HPP
#define WEBSOCKETPP_API_HPP

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include "BSocket.hpp"
#include "Trader.hpp"
#include "utility.hpp"
#include <nlohmann/json.hpp>

class Trader; // forward declare

class Api {
public:
    // Constructor takes a connected WebSocket (BSocket implementation)
    Api(BSocket* socket);
    ~Api();

    // Connect and authenticate (if credentials provided) using the underlying socket
    bool connect(const std::string& url);
    bool authenticate(const std::string& client_id, const std::string& client_secret);
    bool subscribePublic(const std::string& channel);
    bool subscribePrivate(const std::string& channel);
    bool placeOrder(const std::string& instrument, const std::string& side, double price, double amount);
    bool cancelOrder(const std::string& order_id);
    bool editOrder(const std::string& order_id, double newPrice, double newAmount);
    bool getOrderBook(const std::string& instrument);
    bool getPositions(const std::string& currency);

    // Set trader callback for events
    void setTrader(Trader* trader) { this->trader = trader; }

    // Handler for incoming messages (called by BSocket)
    void onMessage(const std::string& message);

private:
    BSocket* socket;
    Trader* trader;
    std::string accessToken;
    std::atomic<int> requestIdCounter;

    // Data structures for tracking
    struct Position { std::string instrument; double size; double average_price; };
    std::vector<Position> positions;
    struct OrderBook {
        std::map<double,double,std::greater<double>> bids;
        std::map<double,double,std::less<double>> asks;
    } orderBook;

    // Track pending request types and timestamps for latency measurement
    struct RequestInfo { std::string type; std::chrono::high_resolution_clock::time_point sentTime; };
    std::mutex reqMutex;
    std::unordered_map<int, RequestInfo> pendingRequests;

    // For linking order responses to trigger events (end-to-end latency)
    std::unordered_map<int, std::chrono::high_resolution_clock::time_point> triggerEventTime;

    // Utility for logging JSON events
    void logJsonEvent(const nlohmann::json& j);
};

#endif 