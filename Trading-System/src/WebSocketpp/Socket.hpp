#ifndef WEBSOCKETPP_SOCKET_HPP
#define WEBSOCKETPP_SOCKET_HPP

#include "BSocket.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <thread>
#include <mutex>

class Socket : public BSocket {
public:
    Socket();
    ~Socket() override;
    bool connect(const std::string& url) override;
    void send(const std::string& message) override;
    void close() override;
private:
    void doRead();
    // Boost Beast WebSocket over TLS
    using tcp = boost::asio::ip::tcp;
    namespace websocket = boost::beast::websocket;
    namespace ssl = boost::asio::ssl;
    boost::asio::io_context ioc;
    ssl::context sslCtx;
    websocket::stream<boost::beast::ssl_stream<tcp::socket>> ws;
    std::thread ioThread;
    std::mutex writeMutex;
    bool open;
};

#endif // WEBSOCKETPP_SOCKET_HPP