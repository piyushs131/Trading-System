#include "Api.hpp"
#include "Trader.hpp"
#include <iostream>
#include <cmath>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

Api::Api(BSocket* socket)
    : socket(socket), trader(nullptr) {
    requestIdCounter = 1;
    // Set this Api's onMessage as the socket callback
    socket->setMessageHandler([this](const std::string& msg) {
        this->onMessage(msg);
    });
}

Api::~Api() {
    // Ensure socket closed and freed
    if (socket) {
        socket->close();
        delete socket;
        socket = nullptr;
    }
}

bool Api::connect(const std::string& url) {
    if (!socket->connect(url)) {
        std::cerr << "Failed to connect to " << url << std::endl;
        return false;
    }
    return true;
}

bool Api::authenticate(const std::string& client_id, const std::string& client_secret) {
    if (client_id.empty() || client_secret.empty()) {
        return false;
    }
    // Build auth request
    int id = requestIdCounter.fetch_add(1);
    json authReq = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "public/auth"},
        {"params", {
            {"grant_type", "client_credentials"},
            {"client_id", client_id},
            {"client_secret", client_secret}
        }}
    };
    {
        std::lock_guard<std::mutex> lock(reqMutex);
        pendingRequests[id] = {"auth", std::chrono::high_resolution_clock::now()};
    }
    std::string msg = authReq.dump();
    socket->send(msg);
    return true;
}

bool Api::subscribePublic(const std::string& channel) {
    int id = requestIdCounter.fetch_add(1);
    json req = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "public/subscribe"},
        {"params", { {"channels", {channel}} }}
    };
    {
        std::lock_guard<std::mutex> lock(reqMutex);
        pendingRequests[id] = {"subscribe", std::chrono::high_resolution_clock::now()};
    }
    return socket->send(req.dump());
}

bool Api::subscribePrivate(const std::string& channel) {
    int id = requestIdCounter.fetch_add(1);
    json req = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "private/subscribe"},
        {"params", { {"channels", {channel}} }}
    };
    // Note: require authentication done (accessToken not explicitly needed in subscribe call after auth)
    {
        std::lock_guard<std::mutex> lock(reqMutex);
        pendingRequests[id] = {"subscribe", std::chrono::high_resolution_clock::now()};
    }
    return socket->send(req.dump());
}

bool Api::placeOrder(const std::string& instrument, const std::string& side, double price, double amount) {
    int id = requestIdCounter.fetch_add(1);
    std::string method = (side == "buy") ? "private/buy" : "private/sell";
    json params = {
        {"instrument_name", instrument},
        {"amount", amount},
        {"type", "limit"},
        {"price", price}
    };
    if (!accessToken.empty()) {
        params["access_token"] = accessToken;
    }
    json req = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", method},
        {"params", params}
    };
    {
        std::lock_guard<std::mutex> lock(reqMutex);
        pendingRequests[id] = {"order", std::chrono::high_resolution_clock::now()};
    }
    bool sent = socket->send(req.dump());
    return sent;
}

bool Api::cancelOrder(const std::string& order_id) {
    int id = requestIdCounter.fetch_add(1);
    json params = { {"order_id", order_id} };
    if (!accessToken.empty()) params["access_token"] = accessToken;
    json req = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "private/cancel"},
        {"params", params}
    };
    {
        std::lock_guard<std::mutex> lock(reqMutex);
        pendingRequests[id] = {"cancel", std::chrono::high_resolution_clock::now()};
    }
    return socket->send(req.dump());
}

bool Api::editOrder(const std::string& order_id, double newPrice, double newAmount) {
    int id = requestIdCounter.fetch_add(1);
    json params = {
        {"order_id", order_id},
        {"amount", newAmount},
        {"price", newPrice}
    };
    if (!accessToken.empty()) params["access_token"] = accessToken;
    json req = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "private/edit"},
        {"params", params}
    };
    {
        std::lock_guard<std::mutex> lock(reqMutex);
        pendingRequests[id] = {"edit", std::chrono::high_resolution_clock::now()};
    }
    return socket->send(req.dump());
}

bool Api::getOrderBook(const std::string& instrument) {
    int id = requestIdCounter.fetch_add(1);
    json req = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "public/get_order_book"},
        {"params", { {"instrument_name", instrument}, {"depth", 10} }}
    };
    {
        std::lock_guard<std::mutex> lock(reqMutex);
        pendingRequests[id] = {"get_order_book", std::chrono::high_resolution_clock::now()};
    }
    return socket->send(req.dump());
}

bool Api::getPositions(const std::string& currency) {
    int id = requestIdCounter.fetch_add(1);
    json params = { {"currency", currency} };
    if (!accessToken.empty()) params["access_token"] = accessToken;
    json req = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", "private/get_positions"},
        {"params", params}
    };
    {
        std::lock_guard<std::mutex> lock(reqMutex);
        pendingRequests[id] = {"get_positions", std::chrono::high_resolution_clock::now()};
    }
    return socket->send(req.dump());
}

void Api::logJsonEvent(const json& j) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << j.dump() << std::endl;
}

void Api::onMessage(const std::string& message) {
    // Record receive time for latency measurements
    auto receiveTime = std::chrono::high_resolution_clock::now();
    // Parse message to JSON
    json msgJson;
    try {
        msgJson = json::parse(message);
    } catch (...) {
        std::cerr << "Failed to parse incoming message as JSON: " << message << std::endl;
        return;
    }
    // If this is a subscription update (no id, has method)
    if (msgJson.contains("method") && msgJson["method"] == "subscription") {
        std::string channel = msgJson["params"]["channel"];
        // Market data update (order book changes)
        if (channel.rfind("book.", 0) == 0) {
            // Calculate propagation delay if possible
            long propagation_ms = 0;
            if (msgJson["params"]["data"].contains("timestamp")) {
                // timestamp is in milliseconds
                long evtTs = msgJson["params"]["data"]["timestamp"];
                auto evtTimePoint = std::chrono::system_clock::time_point(std::chrono::milliseconds(evtTs));
                auto evtTimeLocal = std::chrono::time_point_cast<std::chrono::milliseconds>(evtTimePoint).time_since_epoch().count();
                auto recvTimeLocal = std::chrono::time_point_cast<std::chrono::milliseconds>(receiveTime).time_since_epoch().count();
                propagation_ms = recvTimeLocal - evtTimeLocal;
            }
            // Update internal order book snapshot (replace with latest top levels)
            orderBook.bids.clear();
            orderBook.asks.clear();
            if (msgJson["params"]["data"].contains("bids")) {
                for (auto& level : msgJson["params"]["data"]["bids"]) {
                    double price = level[0];
                    double size = level[1];
                    if (size > 1e-12) {
                        orderBook.bids[price] = size;
                    }
                }
            }
            if (msgJson["params"]["data"].contains("asks")) {
                for (auto& level : msgJson["params"]["data"]["asks"]) {
                    double price = level[0];
                    double size = level[1];
                    if (size > 1e-12) {
                        orderBook.asks[price] = size;
                    }
                }
            }
            // Calculate processing latency (time to handle this message)
            auto afterProcess = std::chrono::high_resolution_clock::now();
            auto process_us = std::chrono::duration_cast<std::chrono::microseconds>(afterProcess - receiveTime).count();
            double process_ms = process_us / 1000.0;
            // Log market update
            json logEvent = {
                {"event", "market_update"},
                {"channel", channel},
                {"propagation_ms", propagation_ms},
                {"process_ms", process_ms}
            };
            if (!orderBook.bids.empty()) logEvent["best_bid"] = orderBook.bids.begin()->first;
            if (!orderBook.asks.empty()) logEvent["best_ask"] = orderBook.asks.begin()->first;
            logJsonEvent(logEvent);
            // Notify trader about book update
            if (trader) {
                trader->onOrderBookUpdate(orderBook);
            }
        }
        // User order update (if subscribed)
        else if (channel.rfind("user.orders", 0) == 0) {
            auto data = msgJson["params"]["data"];
            std::string ordId = data.value("order_id", "");
            std::string state = data.value("order_state", "");
            json logEvent = {
                {"event", "order_update"},
                {"order_id", ordId},
                {"order_state", state}
            };
            if (data.contains("filled_amount")) {
                logEvent["filled_amount"] = data["filled_amount"];
            }
            logJsonEvent(logEvent);
            if (trader) {
                if (state == "filled" || state == "cancelled" || state == "rejected") {
                    trader->onOrderClosed(ordId);
                }
            }
        }
        return;
    }
    // If this is a response to a request (has id)
    if (msgJson.contains("id")) {
        int respId = msgJson["id"];
        // Locate request info
        RequestInfo reqInfo;
        {
            std::lock_guard<std::mutex> lock(reqMutex);
            if (pendingRequests.find(respId) != pendingRequests.end()) {
                reqInfo = pendingRequests[respId];
                pendingRequests.erase(respId);
            } else {
                // Unknown request id (maybe timed out or already handled)
                reqInfo.type = "";
            }
        }
        // Handle error if present
        if (msgJson.contains("error") && !msgJson["error"].is_null()) {
            std::string errorMsg = msgJson["error"].dump();
            json logEvent = { {"event", "error"}, {"type", reqInfo.type}, {"details", errorMsg} };
            logJsonEvent(logEvent);
            return;
        }
        // Compute latency for this response
        double latency_ms = 0.0;
        if (!reqInfo.type.empty()) {
            auto sentTime = reqInfo.sentTime;
            auto now = std::chrono::high_resolution_clock::now();
            latency_ms = std::chrono::duration<double, std::milli>(now - sentTime).count();
        }
        // Specific handling by request type
        if (reqInfo.type == "auth") {
            if (msgJson.contains("result") && msgJson["result"].contains("access_token")) {
                accessToken = msgJson["result"]["access_token"];
                json logEvent = { {"event", "auth_success"}, {"latency_ms", latency_ms} };
                logJsonEvent(logEvent);
            } else {
                json logEvent = { {"event", "auth_failed"} };
                logJsonEvent(logEvent);
            }
        }
        else if (reqInfo.type == "order") {
            // Log order ack and latency
            std::string orderId = "";
            std::string orderState = "";
            if (msgJson.contains("result")) {
                auto result = msgJson["result"];
                if (result.contains("order")) {
                    orderId = result["order"]["order_id"].get<std::string>();
                    orderState = result["order"]["order_state"].get<std::string>();
                }
            }
            json logEvent = {
                {"event", "order_ack"},
                {"order_id", orderId},
                {"order_state", orderState},
                {"latency_ms", latency_ms}
            };
            // If this order was triggered by a market event, measure end-to-end latency
            if (triggerEventTime.find(respId) != triggerEventTime.end()) {
                auto trigTime = triggerEventTime[respId];
                triggerEventTime.erase(respId);
                double loopLatency = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - trigTime).count();
                logEvent["trading_loop_latency_ms"] = loopLatency;
            }
            logJsonEvent(logEvent);
            // If order is open, inform Trader
            if (trader && !orderId.empty() && orderState == "open") {
                trader->onOrderOpen(orderId);
            }
        }
        else if (reqInfo.type == "cancel") {
            json logEvent = { {"event", "cancel_ack"}, {"latency_ms", latency_ms} };
            logJsonEvent(logEvent);
        }
        else if (reqInfo.type == "edit") {
            json logEvent = { {"event", "edit_ack"}, {"latency_ms", latency_ms} };
            logJsonEvent(logEvent);
        }
        else if (reqInfo.type == "get_order_book") {
            // Update internal order book from result
            orderBook.bids.clear();
            orderBook.asks.clear();
            if (msgJson.contains("result")) {
                auto result = msgJson["result"];
                if (result.contains("bids")) {
                    for (auto& level : result["bids"]) {
                        double price = level[0];
                        double size = level[1];
                        orderBook.bids[price] = size;
                    }
                }
                if (result.contains("asks")) {
                    for (auto& level : result["asks"]) {
                        double price = level[0];
                        double size = level[1];
                        orderBook.asks[price] = size;
                    }
                }
            }
            json logEvent = { {"event", "order_book_snapshot"}, {"latency_ms", latency_ms} };
            if (!orderBook.bids.empty()) logEvent["best_bid"] = orderBook.bids.begin()->first;
            if (!orderBook.asks.empty()) logEvent["best_ask"] = orderBook.asks.begin()->first;
            logJsonEvent(logEvent);
        }
        else if (reqInfo.type == "get_positions") {
            positions.clear();
            if (msgJson.contains("result")) {
                for (auto& pos : msgJson["result"]) {
                    Position p;
                    p.instrument = pos.value("instrument_name", "");
                    p.size = pos.value("size", 0.0);
                    p.average_price = pos.value("average_price", 0.0);
                    positions.push_back(p);
                }
            }
            json logEvent = { {"event", "positions_snapshot"}, {"latency_ms", latency_ms} };
            for (auto& p : positions) {
                json pj = { {"instrument", p.instrument}, {"size", p.size}, {"avg_price", p.average_price} };
                logEvent["positions"].push_back(pj);
            }
            logJsonEvent(logEvent);
        }
        // subscribe ack (we don't explicitly log unless needed)
        else if (reqInfo.type == "subscribe") {
            json logEvent = { {"event", "subscribe_ack"}, {"latency_ms", latency_ms} };
            logJsonEvent(logEvent);
        }
    }
}