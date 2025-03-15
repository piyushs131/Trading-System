#ifndef TEST_LATENCY_SOCKET_HPP
#define TEST_LATENCY_SOCKET_HPP

#include <string>
#include <boost/asio.hpp>

/**
 * Socket wraps a raw TCP socket for synchronous send/receive (used by Api).
 */
class Socket {
public:
    Socket();
    ~Socket();
    bool connect(const std::string& host, unsigned short port);
    bool send(const std::string& message);
    bool receive(std::string& outMessage);
    void close();
private:
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::socket socket;
    bool connected;
};

#endif // TEST_LATENCY_SOCKET_HPP