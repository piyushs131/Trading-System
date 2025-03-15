#ifndef CPARSER_HPP
#define CPARSER_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

class CParser {
public:
    // Constructor & Destructor
    CParser();
    ~CParser();

    // Parses incoming WebSocket JSON messages
    nlohmann::json parseMessage(const std::string& rawMessage);

    // Extracts order book data from JSON response
    std::vector<std::pair<double, double>> extractOrderBook(const nlohmann::json& parsedJson);

    // Extracts trade updates from JSON response
    std::vector<std::string> extractTradeUpdates(const nlohmann::json& parsedJson);

    // Handles errors from WebSocket messages
    std::string handleWebSocketError(const nlohmann::json& errorJson);

private:
    // Helper function to validate JSON
    bool isValidJson(const std::string& rawMessage);
};

#endif 