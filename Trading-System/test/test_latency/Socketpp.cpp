#include "Socketpp.hpp"
#include "utility.hpp"
#include <iostream>
#include <boost/asio/connect.hpp>
#include <boost/asio/write.hpp>
#include <array>

namespace net = boost::asio;
using tcp = net::ip::tcp;

Socketpp::Socketpp() : ioc(), socket(ioc), connected(false) {
}

Socketpp::~Socketpp() {
    close();
}

bool Socketpp::connect(const std::string& host, unsigned short port) {
    try {
        tcp::resolver resolver(ioc);
        boost::system::error_code ec;
        auto endpoints = resolver.resolve(host, std::to_string(port), ec);
        if (ec) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"socket_error\",\"detail\":\"resolve: " << ec.message() << "\"}" << std::endl;
            return false;
        }
        net::connect(socket, endpoints, ec);
        if (ec) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "{\"event\":\"socket_error\",\"detail\":\"connect: " << ec.message() << "\"}" << std::endl;
            return false;
        }
        connected = true;
        return true;
    } catch (const std::exception& ex) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"socket_error\",\"detail\":\"exception: " << ex.what() << "\"}" << std::endl;
        return false;
    }
}

bool Socketpp::send(const std::string& message) {
    if (!connected) return false;
    boost::system::error_code ec;
    if (!message.empty()) {
        // Use scatter-gather write to avoid extra string copy for newline
        std::array<net::const_buffer, 2> buffers = {
            net::buffer(message),
            net::buffer("\n", 1)
        };
        net::write(socket, buffers, ec);
    } else {
        // Send just newline if message is empty
        net::write(socket, net::buffer("\n", 1), ec);
    }
    if (ec) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"socket_error\",\"detail\":\"send: " << ec.message() << "\"}" << std::endl;
        return false;
    }
    return true;
}

bool Socketpp::receive(std::string& outMessage) {
    if (!connected) return false;
    boost::asio::streambuf buf;
    boost::system::error_code ec;
    net::read_until(socket, buf, '\n', ec);
    if (ec && ec != net::error::eof) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"socket_error\",\"detail\":\"recv: " << ec.message() << "\"}" << std::endl;
        return false;
    }
    std::istream is(&buf);
    std::string line;
    std::getline(is, line);
    outMessage = line;
    return true;
}

void Socketpp::close() {
    if (connected) {
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        socket.close(ec);
        connected = false;
    }
}