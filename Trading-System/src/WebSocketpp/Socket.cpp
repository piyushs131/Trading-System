#include "Socket.hpp"
#include <iostream>

Socket::Socket()
    : sslCtx(ssl::context::tlsv12_client), ws(ioc, sslCtx), open(false) {
    sslCtx.set_verify_mode(boost::asio::ssl::verify_none);
}

Socket::~Socket() {
    close();
}

bool Socket::connect(const std::string& url) {
    try {
        std::string uri = url;
        std::string host = uri;
        std::string path = "/";
        // Strip scheme
        if (host.rfind("wss://", 0) == 0) host = host.substr(6);
        if (host.rfind("ws://", 0) == 0) host = host.substr(5);
        // Split host and path
        size_t pos = host.find('/');
        if (pos != std::string::npos) {
            path = host.substr(pos);
            host = host.substr(0, pos);
        }
        std::string port = "443";
        // Resolve and connect TCP
        tcp::resolver resolver(ioc);
        auto results = resolver.resolve(host, port);
        boost::beast::get_lowest_layer(ws).connect(results);
        // SSL handshake
        ws.next_layer().handshake(ssl::stream_base::client);
        // WebSocket handshake
        ws.set_option(websocket::stream_base::timeout::suggested(websocket::role_type::client));
        ws.handshake(host, path);
        ws.text(true);
        open = true;
        // Start asynchronous read loop
        doRead();
        ioThread = std::thread([this]() {
            try {
                ioc.run();
            } catch (const std::exception& e) {
                std::cerr << "Socket io_thread exception: " << e.what() << std::endl;
            }
        });
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "Socket connect error: " << ex.what() << std::endl;
        open = false;
        return false;
    }
}

void Socket::doRead() {
    // Allocate a buffer for each read
    auto buffer = std::make_shared<boost::beast::flat_buffer>();
    ws.async_read(*buffer, [this, buffer](boost::beast::error_code ec, std::size_t bytes) {
        if (!ec) {
            std::string message((char*)buffer->data().data(), bytes);
            if (messageHandler) {
                messageHandler(message);
            }
            doRead(); // continue reading next message
        } else {
            if (ec != websocket::error::closed) {
                std::cerr << "Socket read error: " << ec.message() << std::endl;
            }
            open = false;
        }
    });
}

void Socket::send(const std::string& message) {
    if (!open) return;
    // Post send task to I/O context for thread safety
    boost::asio::post(ioc, [this, message]() {
        std::lock_guard<std::mutex> lock(writeMutex);
        boost::beast::error_code ec;
        ws.write(boost::asio::buffer(message), ec);
        if (ec) {
            std::cerr << "Socket send error: " << ec.message() << std::endl;
        }
    });
}

void Socket::close() {
    if (!open) {
        if (ioThread.joinable()) ioThread.join();
        return;
    }
    // Signal closure on I/O thread
    boost::asio::post(ioc, [this]() {
        boost::beast::error_code ec;
        ws.close(websocket::close_code::normal, ec);
    });
    ioc.stop();
    if (ioThread.joinable()) {
        ioThread.join();
    }
    open = false;
}