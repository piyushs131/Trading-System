#ifndef TEST_THROUGHPUT_SOCKETPP_HPP
#define TEST_THROUGHPUT_SOCKETPP_HPP

#include <string>
#include <boost/asio.hpp>

/**
 * Socketpp (throughput version).
 * Not used in throughput test, provided for completeness.
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

#endif // TEST_THROUGHPUT_SOCKETPP_HPP