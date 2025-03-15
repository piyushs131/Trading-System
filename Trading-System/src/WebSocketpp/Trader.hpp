#ifndef WEBSOCKETPP_TRADER_HPP
#define WEBSOCKETPP_TRADER_HPP

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>

class Api; // forward declaration

class Trader {
public:
    Trader(Api* api);
    ~Trader();

    // Start trading logic: subscribe to data and private channels
    void start();

    // Callback from Api when order book updates
    template<typename OrderBookType>
    void onOrderBookUpdate(const OrderBookType& book);

    // Callback from Api when an order is confirmed open
    void onOrderOpen(const std::string& order_id);
    // Callback from Api when an order is closed (filled or cancelled)
    void onOrderClosed(const std::string& order_id);

private:
    Api* api;
    std::thread monitorThread;
    std::atomic<bool> running;
    std::mutex ordersMutex;
    struct OpenOrder { std::string id; std::chrono::steady_clock::time_point time; };
    std::vector<OpenOrder> openOrders;
};

#endif // WEBSOCKETPP_TRADER_HPP