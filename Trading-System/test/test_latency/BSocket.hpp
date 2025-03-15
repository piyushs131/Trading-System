#ifndef TEST_LATENCY_BSOCKET_HPP
#define TEST_LATENCY_BSOCKET_HPP

#include <string>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

/**
 * BSocket wraps a Boost.Beast WebSocket client for sending/receiving messages.
 */
class BSocket {
public:
    BSocket();
    ~BSocket();
    bool connect(const std::string& host, unsigned short port);
    bool send(const std::string& message);
    bool receive(std::string& outMessage);
    void close();
private:
    boost::asio::io_context ioc;
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws;
    bool connected;
};

#endif // TEST_LATENCY_BSOCKET_HPP

