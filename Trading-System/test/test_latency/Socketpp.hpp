#ifndef TEST_LATENCY_SOCKETPP_HPP
#define TEST_LATENCY_SOCKETPP_HPP

#include <string>
#include <boost/asio.hpp>

/**
 * Socketpp (Socket++) is an enhanced TCP socket wrapper.
 * In this context, it provides similar functionality to Socket.
 */
class Socketpp {
public:
    Socketpp();
    ~Socketpp();
    bool connect(const std::string& host, unsigned short port);
    bool send(const std::string& message);
    bool receive(std::string& outMessage);
    void close();
private:
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::socket socket;
    bool connected;
};

#endif // TEST_LATENCY_SOCKETPP_HPP