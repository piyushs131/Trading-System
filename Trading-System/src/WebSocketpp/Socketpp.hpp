#ifndef WEBSOCKETPP_SOCKETPP_HPP
#define WEBSOCKETPP_SOCKETPP_HPP

#include "BSocket.hpp"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>

class Socketpp : public BSocket {
public:
    Socketpp();
    ~Socketpp() override;
    bool connect(const std::string& url) override;
    void send(const std::string& message) override;
    void close() override;
private:
    // WebSocket++ client with TLS
    using Client = websocketpp::client<websocketpp::config::asio_tls_client>;
    Client endpoint;
    websocketpp::connection_hdl connHdl;
    std::thread ioThread;
    std::mutex connectMutex;
    std::condition_variable connectCond;
    bool connected;

    // TLS initialization callback
    std::shared_ptr<boost::asio::ssl::context> onTlsInit();
};

#endif // WEBSOCKETPP_SOCKETPP_HPP