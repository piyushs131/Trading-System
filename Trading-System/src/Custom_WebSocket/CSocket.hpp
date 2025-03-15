#ifndef CSOCKET_HPP
#define CSOCKET_HPP

#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include "../Custom_WebSocket/CParser.hpp"

using boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

class CSocket {
public:
    CSocket(const std::string& host, const std::string& port);
    ~CSocket();

    void connect();
    void sendMessage(const std::string& message);
    void listen();
    void close();

private:
    std::string host_;
    std::string port_;
    boost::asio::io_context io_context_;
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    std::thread listener_thread_;
    CParser parser_;

    void handleIncomingMessage(const std::string& message);
};

#endif // CSOCKET_HPP