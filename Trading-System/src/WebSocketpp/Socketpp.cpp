#include "Socketpp.hpp"
#include <iostream>
#include <boost/asio.hpp>

Socketpp::Socketpp() : connected(false) {
    // Initialize endpoint
    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.clear_error_channels(websocketpp::log::elevel::all);
    endpoint.init_asio();
    // TLS handler
    endpoint.set_tls_init_handler([this](websocketpp::connection_hdl) {
        return onTlsInit();
    });
    // Message handler
    endpoint.set_message_handler([this](websocketpp::connection_hdl, Client::message_ptr msg) {
        if (messageHandler) {
            messageHandler(msg->get_payload());
        }
    });
    // Open handler
    endpoint.set_open_handler([this](websocketpp::connection_hdl hdl) {
        connHdl = hdl;
        {
            std::lock_guard<std::mutex> lock(connectMutex);
            connected = true;
        }
        connectCond.notify_one();
    });
    // Fail handler
    endpoint.set_fail_handler([this](websocketpp::connection_hdl) {
        std::lock_guard<std::mutex> lock(connectMutex);
        connected = false;
        connectCond.notify_one();
    });
    // Close handler
    endpoint.set_close_handler([this](websocketpp::connection_hdl) {
        std::lock_guard<std::mutex> lock(connectMutex);
        connected = false;
    });
}

Socketpp::~Socketpp() {
    close();
}

std::shared_ptr<boost::asio::ssl::context> Socketpp::onTlsInit() {
    auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds);
        ctx->set_verify_mode(boost::asio::ssl::verify_none);
    } catch (...) {
        std::cerr << "TLS init error\n";
    }
    return ctx;
}

bool Socketpp::connect(const std::string& url) {
    websocketpp::lib::error_code ec;
    Client::connection_ptr con = endpoint.get_connection(url, ec);
    if (ec) {
        std::cerr << "Socketpp connection error: " << ec.message() << std::endl;
        return false;
    }
    endpoint.connect(con);
    // Run networking in separate thread
    ioThread = std::thread([this]() {
        endpoint.run();
    });
    // Wait for open or fail
    std::unique_lock<std::mutex> lock(connectMutex);
    connectCond.wait(lock, [this] { return connected; });
    return connected;
}

void Socketpp::send(const std::string& message) {
    if (!connected) return;
    websocketpp::lib::error_code ec;
    // Post the send to the endpoint's io_service to ensure thread safety
    endpoint.get_io_service()->post([this, message]() {
        websocketpp::lib::error_code e;
        endpoint.send(connHdl, message, websocketpp::frame::opcode::text, e);
        if (e) {
            std::cerr << "Socketpp send error: " << e.message() << std::endl;
        }
    });
}

void Socketpp::close() {
    if (!connected) {
        if (ioThread.joinable()) ioThread.join();
        return;
    }
    // Close connection
    websocketpp::lib::error_code ec;
    endpoint.close(connHdl, websocketpp::close::status::normal, "", ec);
    if (ec) {
        std::cerr << "Socketpp close error: " << ec.message() << std::endl;
    }
    if (ioThread.joinable()) {
        ioThread.join();
    }
    connected = false;
}