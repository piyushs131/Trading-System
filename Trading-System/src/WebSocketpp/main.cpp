#include "../Custom_WebSocket/CSocket.hpp"
#include "Api.hpp"
#include "Trader.hpp"
#include "Socketpp.hpp"
#include "Socket.hpp"
#include "utility.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

int main() {
    try {
        // ✅ Initialize WebSocket client using Custom WebSocket (Boost.Beast)
        CSocket wsClient("test.deribit.com", "443");

        // ✅ Initialize API with WebSocket
        Api api(&wsClient);
        Trader trader(&api);

        // ✅ Connect to Deribit testnet WebSocket
        std::string url = "wss://test.deribit.com/ws/api/v2";
        if (!wsClient.connect()) {
            std::cerr << "❌ Connection failed, exiting.\n";
            return 1;
        }

        // ✅ Authenticate with API credentials (optional)
        std::string client_id = "YOUR_CLIENT_ID";    // Set your testnet client_id
        std::string client_secret = "YOUR_CLIENT_SECRET"; // Set your testnet client_secret
        if (!client_id.empty() && !client_secret.empty()) {
            api.authenticate(client_id, client_secret);
            std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait for authentication
        }

        // ✅ Subscribe to order book & trade updates
        std::string subscribeMessage = R"({
            "jsonrpc": "2.0",
            "method": "public/subscribe",
            "params": {
                "channels": ["book.BTC-PERP.100ms", "trades.BTC-PERP"]
            }
        })";

        wsClient.sendMessage(subscribeMessage);
        std::cout << "✅ Subscribed to BTC-PERP market data.\n";

        // ✅ Link trader to API and start trading logic
        api.setTrader(&trader);
        trader.start();

        // ✅ Keep the system running for a limited time (or indefinitely)
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "🔄 Shutting down trading system...\n";

        // ✅ Close API connection
        api.close();

    } catch (const std::exception& e) {
        std::cerr << "❌ Error in main: " << e.what() << std::endl;
    }

    return 0;
}