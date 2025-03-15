#include "Trader.hpp"
#include "Api.hpp"
#include <chrono>
#include <iostream>

Trader::Trader(Api* api) : api(api), running(false) {}

Trader::~Trader() {
    running = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
}

void Trader::start() {
    // Subscribe to public order book and user orders (private)
    api->subscribePublic("book.BTC-PERPETUAL.raw");    // high-frequency raw updates
    api->subscribePrivate("user.orders.BTC-PERPETUAL.raw");
    // Get initial positions and order book snapshot
    api->getPositions("BTC");
    api->getOrderBook("BTC-PERPETUAL");
    // Start monitor thread for stale order cancellation
    running = true;
    monitorThread = std::thread([this]() {
        const int cancelTimeoutSec = 5;
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto now = std::chrono::steady_clock::now();
            std::vector<std::string> toCancel;
            {
                std::lock_guard<std::mutex> lock(ordersMutex);
                for (auto it = openOrders.begin(); it != openOrders.end();) {
                    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->time).count();
                    if (age >= cancelTimeoutSec) {
                        toCancel.push_back(it->id);
                        it = openOrders.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            for (auto& oid : toCancel) {
                std::cout << "Cancelling stale order: " << oid << std::endl;
                api->cancelOrder(oid);
            }
        }
    });
}

template<typename OrderBookType>
void Trader::onOrderBookUpdate(const OrderBookType& book) {
    // Simple strategy: if spread is wide, place buy and sell orders
    if (book.bids.empty() || book.asks.empty()) {
        return;
    }
    double bestBid = book.bids.begin()->first;
    double bestAsk = book.asks.begin()->first;
    double spread = bestAsk - bestBid;
    // Example threshold for placing orders
    if (spread > 10.0) {
        // Only place if no current open orders
        std::lock_guard<std::mutex> lock(ordersMutex);
        if (openOrders.empty()) {
            double buyPrice = bestBid + 0.5;
            double sellPrice = bestAsk - 0.5;
            double qty = 10.0;
            std::cout << "Placing buy at " << buyPrice << " and sell at " << sellPrice 
                      << " (spread " << spread << ")\n";
            api->placeOrder("BTC-PERPETUAL", "buy", buyPrice, qty);
            api->placeOrder("BTC-PERPETUAL", "sell", sellPrice, qty);
            // We will add to openOrders when ack is received in onOrderOpen
        }
    }
}

// Explicitly instantiate template for the expected OrderBook type (map<double,double>)
template void Trader::onOrderBookUpdate<const Api::OrderBook&>(const Api::OrderBook& book);

void Trader::onOrderOpen(const std::string& order_id) {
    // Record the open order with current time
    std::lock_guard<std::mutex> lock(ordersMutex);
    OpenOrder o{order_id, std::chrono::steady_clock::now()};
    openOrders.push_back(o);
}

void Trader::onOrderClosed(const std::string& order_id) {
    // Remove order from openOrders
    std::lock_guard<std::mutex> lock(ordersMutex);
    openOrders.erase(std::remove_if(openOrders.begin(), openOrders.end(),
                    [&](const OpenOrder& o){ return o.id == order_id; }), openOrders.end());
}