#include <iostream>
#include <thread>
#include <vector>
#include <deque>
#include <condition_variable>
#include <limits>
#include <algorithm>
#include <string>
#include <chrono>
#include <atomic>
#include "Api.hpp"
#include "BSocket.hpp"
#include "utility.hpp"

namespace net = boost::asio;
namespace beast = boost::beast;
using tcp = net::ip::tcp;

// Structure for market data events (used in end-to-end test)
struct MarketEvent {
    std::string symbol;
    double price;
    long long timestamp_ns;
};

int main() {
    // Prepare symbols and initial prices for market data
    std::vector<std::string> symbols = {"SYM0", "SYM1", "SYM2", "SYM3", "SYM4"};
    std::unordered_map<std::string, double> priceMap;
    for (const auto& sym : symbols) {
        priceMap[sym] = 100.0;
    }

    // Launch a local TCP server to simulate the order API (for latency tests)
    std::thread orderServerThread([&]() {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), ORDER_SERVER_PORT));
        tcp::socket clientSock(ioc);
        boost::system::error_code ec;
        acceptor.accept(clientSock, ec);
        if (ec) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"order_server_error\",\"detail\":\"accept: " << ec.message() << "\"}" << std::endl;
            return;
        }
        for (;;) {
            boost::asio::streambuf buf;
            size_t n = net::read_until(clientSock, buf, '\n', ec);
            if (ec) {
                // Break on disconnect or error
                break;
            }
            std::istream is(&buf);
            std::string request;
            std::getline(is, request);
            if (request.empty()) continue;
            // Process commands: "LOGIN" or "ORDER"
            std::string response;
            if (request.rfind("LOGIN", 0) == 0) {
                response = "LOGIN_OK";
            } else if (request.rfind("ORDER", 0) == 0) {
                response = "ORDER_ACK" + request.substr(5);
            } else {
                response = request; // echo unknown commands
            }
            // Send response back to client
            std::string respLine = response + "\n";
            net::write(clientSock, net::buffer(respLine), ec);
            if (ec) break;
        }
    });

    // Launch a local WebSocket server to simulate market data feed (echo server)
    std::thread wsServerThread([&]() {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), WEBSOCKET_PORT));
        tcp::socket sock(ioc);
        boost::system::error_code ec;
        acceptor.accept(sock, ec);
        if (ec) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"ws_server_error\",\"detail\":\"accept: " << ec.message() << "\"}" << std::endl;
            return;
        }
        beast::websocket::stream<tcp::socket> ws(std::move(sock));
        ws.accept(ec);
        if (ec) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"ws_server_error\",\"detail\":\"handshake: " << ec.message() << "\"}" << std::endl;
            return;
        }
        ws.text(true);
        for (;;) {
            beast::flat_buffer buffer;
            ws.read(buffer, ec);
            if (ec) {
                if (ec != beast::websocket::error::closed) {
                    std::lock_guard<std::mutex> lock(logMutex);
                    std::cout << "{\"event\":\"ws_server_error\",\"detail\":\"read: " << ec.message() << "\"}" << std::endl;
                }
                break;
            }
            std::string msg = beast::buffers_to_string(buffer.cdata());
            if (ws.got_text()) {
                ws.text(true);
            }
            // Echo the message back to the client
            ws.write(net::buffer(msg), ec);
            if (ec) {
                std::lock_guard<std::mutex> lock(logMutex);
                std::cout << "{\"event\":\"ws_server_error\",\"detail\":\"write: " << ec.message() << "\"}" << std::endl;
                break;
            }
        }
    });

    // Initialize API client and WebSocket client for tests
    Api api;
    BSocket wsClient;
    // Authenticate with mock API
    if (!api.authenticate("testuser", "testpass")) {
        // If auth failed, clean up threads and exit
        wsClient.close();
        if (orderServerThread.joinable()) orderServerThread.join();
        if (wsServerThread.joinable()) wsServerThread.join();
        return 1;
    }
    // Connect WebSocket client to local feed
    bool wsConnected = wsClient.connect("127.0.0.1", WEBSOCKET_PORT);
    if (!wsConnected) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"ws_client_error\",\"detail\":\"connect_failed\"}" << std::endl;
    }

    // 1. Order Placement Latency Test
    {
        int numOrders = 100;
        long long minLat = std::numeric_limits<long long>::max();
        long long maxLat = 0;
        long long sumLat = 0;
        int count = 0;
        for (int i = 0; i < numOrders; ++i) {
            // Create a sample order (alternating BUY/SELL for variety)
            std::string sym = "TEST";
            std::string side = (i % 2 == 0 ? "BUY" : "SELL");
            Order order(0, sym, side, 1, 100.0 + i * 0.1);
            long long start_ns = nowNs();
            int orderId = api.placeOrder(order);
            long long end_ns = nowNs();
            if (orderId < 0) {
                // Stop the test if an order failed
                break;
            }
            long long latency = end_ns - start_ns;
            if (latency < minLat) minLat = latency;
            if (latency > maxLat) maxLat = latency;
            sumLat += latency;
            count += 1;
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"timestamp_ns\":" << end_ns 
                      << ",\"event\":\"order_completed\""
                      << ",\"order_id\":" << orderId 
                      << ",\"latency_ns\":" << latency << "}" << std::endl;
        }
        if (count > 0) {
            long long avgLat = sumLat / count;
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"order_latency_summary\",\"samples\":" << count
                      << ",\"avg_ns\":" << avgLat 
                      << ",\"min_ns\":" << minLat 
                      << ",\"max_ns\":" << maxLat << "}" << std::endl;
        }
    }

    // 2. Market Data Processing Latency Test
    {
        int numMessages = 1000;
        long long minLat = std::numeric_limits<long long>::max();
        long long maxLat = 0;
        long long sumLat = 0;
        for (int j = 0; j < numMessages; ++j) {
            // Simulate an incoming market data update as "SYMBOL,PRICE"
            std::string symbol = symbols[j % symbols.size()];
            double newPrice = 100.0 + (j * 0.1);
            std::string msg = symbol + "," + std::to_string(newPrice);
            long long start_ns = nowNs();
            // Parse the message (split by comma)
            size_t commaPos = msg.find(',');
            std::string sym = msg.substr(0, commaPos);
            double price = std::stod(msg.substr(commaPos + 1));
            // Update internal market data store
            priceMap[sym] = price;
            long long end_ns = nowNs();
            long long latency = end_ns - start_ns;
            if (latency < minLat) minLat = latency;
            if (latency > maxLat) maxLat = latency;
            sumLat += latency;
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"timestamp_ns\":" << end_ns 
                      << ",\"event\":\"marketdata_processed\""
                      << ",\"symbol\":\"" << symbol << "\""
                      << ",\"latency_ns\":" << latency << "}" << std::endl;
        }
        long long count = numMessages;
        long long avgLat = (count > 0 ? sumLat / count : 0);
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"marketdata_latency_summary\",\"samples\":" << numMessages
                  << ",\"avg_ns\":" << avgLat
                  << ",\"min_ns\":" << minLat
                  << ",\"max_ns\":" << maxLat << "}" << std::endl;
    }

    // 3. WebSocket Message Propagation Latency Test
    if (wsConnected) {
        int numMessages = 100;
        long long minLat = std::numeric_limits<long long>::max();
        long long maxLat = 0;
        long long sumLat = 0;
        int count = 0;
        for (int k = 0; k < numMessages; ++k) {
            std::string message = "PING_" + std::to_string(k);
            long long start_ns = nowNs();
            if (!wsClient.send(message)) {
                break;  // stop if send fails
            }
            std::string reply;
            if (!wsClient.receive(reply)) {
                break;  // stop if receive fails (e.g., connection closed)
            }
            long long end_ns = nowNs();
            long long latency = end_ns - start_ns;
            if (latency < minLat) minLat = latency;
            if (latency > maxLat) maxLat = latency;
            sumLat += latency;
            count += 1;
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"timestamp_ns\":" << end_ns 
                      << ",\"event\":\"ws_roundtrip\""
                      << ",\"message\":\"" << message << "\""
                      << ",\"latency_ns\":" << latency << "}" << std::endl;
        }
        if (count > 0) {
            long long avgLat = sumLat / count;
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"ws_latency_summary\",\"samples\":" << count
                      << ",\"avg_ns\":" << avgLat
                      << ",\"min_ns\":" << minLat
                      << ",\"max_ns\":" << maxLat << "}" << std::endl;
        }
    }

    // 4. End-to-End Trading Loop Latency Test
    {
        int numEvents = 100;
        std::deque<MarketEvent> eventQueue;
        std::mutex queueMutex;
        std::condition_variable queueCV;
        std::atomic<bool> doneProducing(false);

        // Trader thread: consumes market events, processes them, and places orders
        std::thread traderThread([&]() {
            long long minLat = std::numeric_limits<long long>::max();
            long long maxLat = 0;
            long long sumLat = 0;
            int count = 0;
            while (true) {
                MarketEvent event;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCV.wait(lock, [&]() { return !eventQueue.empty() || doneProducing.load(); });
                    if (eventQueue.empty() && doneProducing.load()) {
                        break;  // no more events
                    }
                    event = eventQueue.front();
                    eventQueue.pop_front();
                }
                if (event.symbol == "END") {
                    // Sentinel event to signal completion
                    break;
                }
                // Update market data store with new price (simulate processing)
                priceMap[event.symbol] = event.price;
                // Simple trading logic: always place a BUY order for the event symbol
                Order order;
                order.symbol = event.symbol;
                order.side = "BUY";
                order.quantity = 1;
                order.price = event.price;
                // Place order and measure time from event arrival to order ack
                long long orderStart = nowNs();
                int orderId = api.placeOrder(order);
                long long orderEnd = nowNs();
                if (orderId < 0) {
                    continue;  // skip if order failed
                }
                long long latency = orderEnd - event.timestamp_ns;
                if (latency < minLat) minLat = latency;
                if (latency > maxLat) maxLat = latency;
                sumLat += latency;
                count += 1;
                std::lock_guard<std::mutex> lock(logMutex);
                std::cout << "{\"timestamp_ns\":" << orderEnd 
                          << ",\"event\":\"trade_loop_completed\""
                          << ",\"symbol\":\"" << event.symbol << "\""
                          << ",\"order_id\":" << orderId 
                          << ",\"latency_ns\":" << latency << "}" << std::endl;
            }
            if (count > 0) {
                long long avgLat = sumLat / count;
                std::lock_guard<std::mutex> lock(logMutex);
                std::cout << "{\"event\":\"end_to_end_latency_summary\",\"samples\":" << count
                          << ",\"avg_ns\":" << avgLat
                          << ",\"min_ns\":" << minLat
                          << ",\"max_ns\":" << maxLat << "}" << std::endl;
            }
        });

        // Market data producer thread: generates market events and pushes to queue
        std::thread feedThread([&]() {
            for (int i = 0; i < numEvents; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));  // simulate feed interval
                std::string sym = symbols[i % symbols.size()];
                double price = 100.0 + (i * 0.5);
                long long ts = nowNs();
                MarketEvent ev{sym, price, ts};
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    eventQueue.push_back(ev);
                }
                queueCV.notify_one();
            }
            // Send end-of-stream sentinel
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                eventQueue.push_back({"END", 0.0, 0});
            }
            doneProducing.store(true);
            queueCV.notify_one();
        });

        // Wait for both threads to finish
        feedThread.join();
        traderThread.join();
    }

    // Cleanup: close clients and stop servers
    wsClient.close();
    api.disconnect();
    if (wsServerThread.joinable()) wsServerThread.join();
    if (orderServerThread.joinable()) orderServerThread.join();
    return 0;
}