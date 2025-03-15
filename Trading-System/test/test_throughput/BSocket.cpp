#include "BSocket.hpp"
#include "utility.hpp"
#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

BSocket::BSocket() : ioc(), ws(ioc), connected(false) {}

BSocket::~BSocket() {
    close();
}

bool BSocket::connect(const std::string& host, unsigned short port) {
    try {
        tcp::resolver resolver(ioc);
        boost::system::error_code ec;
        auto results = resolver.resolve(host, std::to_string(port), ec);
        if (ec) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"bsocket_error\",\"detail\":\"resolve: " << ec.message() << "\"}" << std::endl;
            return false;
        }
        net::connect(ws.next_layer(), results.begin(), results.end(), ec);
        if (ec) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"bsocket_error\",\"detail\":\"connect: " << ec.message() << "\"}" << std::endl;
            return false;
        }
        ws.handshake(host, "/", ec);
        if (ec) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"bsocket_error\",\"detail\":\"handshake: " << ec.message() << "\"}" << std::endl;
            return false;
        }
        ws.text(true);
        connected = true;
        return true;
    } catch (const std::exception& ex) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"bsocket_error\",\"detail\":\"exception: " << ex.what() << "\"}" << std::endl;
        return false;
    }
}

bool BSocket::send(const std::string& message) {
    if (!connected) return false;
    boost::system::error_code ec;
    ws.write(net::buffer(message), ec);
    if (ec) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"bsocket_error\",\"detail\":\"send: " << ec.message() << "\"}" << std::endl;
        return false;
    }
    return true;
}

bool BSocket::receive(std::string& outMessage) {
    if (!connected) return false;
    beast::flat_buffer buffer;
    boost::system::error_code ec;
    ws.read(buffer, ec);
    if (ec) {
        if (ec != websocket::error::closed) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"bsocket_error\",\"detail\":\"receive: " << ec.message() << "\"}" << std::endl;
        }
        return false;
    }
    outMessage = beast::buffers_to_string(buffer.cdata());
    return true;
}

void BSocket::close() {
    if (connected) {
        boost::system::error_code ec;
        ws.close(websocket::close_code::normal, ec);
        connected = false;
    }
}