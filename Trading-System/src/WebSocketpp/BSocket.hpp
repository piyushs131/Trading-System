#ifndef WEBSOCKETPP_BSOCKET_HPP
#define WEBSOCKETPP_BSOCKET_HPP

#include <string>
#include <functional>

class BSocket {
public:
    virtual ~BSocket() = default;
    virtual bool connect(const std::string& url) = 0;
    virtual void send(const std::string& message) = 0;
    virtual void close() = 0;
    void setMessageHandler(std::function<void(const std::string&)> handler) {
        messageHandler = std::move(handler);
    }
protected:
    std::function<void(const std::string&)> messageHandler;
};

#endif // WEBSOCKETPP_BSOCKET_HPP