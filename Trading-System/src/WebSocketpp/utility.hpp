#ifndef WEBSOCKETPP_UTILITY_HPP
#define WEBSOCKETPP_UTILITY_HPP

#include <mutex>

// Global logging mutex to serialize log output
static std::mutex logMutex;

constexpr const char* DEFAULT_INSTRUMENT = "BTC-PERPETUAL";

#endif // WEBSOCKETPP_UTILITY_HPP