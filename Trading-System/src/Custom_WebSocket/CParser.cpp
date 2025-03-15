#include "CParser.hpp"
#include <iostream>

using json = nlohmann::json;

CParser::CParser() {}

CParser::~CParser() {}

json CParser::parseMessage(const std::string& rawMessage) {
    try {
        if (!isValidJson(rawMessage)) {
            throw std::runtime_error("Invalid JSON format");
        }
        return json::parse(rawMessage);
    } catch (const std::exception& e) {
        std::cerr << "❌ Error parsing message: " << e.what() << std::endl;
        return json();  // Return empty JSON on failure
    }
}

std::vector<std::pair<double, double>> CParser::extractOrderBook(const json& parsedJson) {
    std::vector<std::pair<double, double>> orderBook;

    try {
        if (parsedJson.contains("params") && parsedJson["params"].contains("data")) {
            for (const auto& entry : parsedJson["params"]["data"]) {
                if (entry.contains("price") && entry.contains("amount")) {
                    double price = entry["price"].get<double>();
                    double amount = entry["amount"].get<double>();
                    orderBook.emplace_back(price, amount);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error extracting order book: " << e.what() << std::endl;
    }

    return orderBook;
}

std::vector<std::string> CParser::extractTradeUpdates(const json& parsedJson) {
    std::vector<std::string> tradeUpdates;

    try {
        if (parsedJson.contains("params") && parsedJson["params"].contains("data")) {
            for (const auto& entry : parsedJson["params"]["data"]) {
                if (entry.contains("trade_id") && entry.contains("price") && entry.contains("amount")) {
                    std::string tradeId = entry["trade_id"].get<std::string>();
                    double price = entry["price"].get<double>();
                    double amount = entry["amount"].get<double>();

                    std::string tradeInfo = "Trade ID: " + tradeId + " | Price: " + std::to_string(price) +
                                            " | Amount: " + std::to_string(amount);
                    tradeUpdates.push_back(tradeInfo);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error extracting trade updates: " << e.what() << std::endl;
    }

    return tradeUpdates;
}

std::string CParser::handleWebSocketError(const json& errorJson) {
    try {
        if (errorJson.contains("error")) {
            std::string errorMessage = errorJson["error"]["message"].get<std::string>();
            int errorCode = errorJson["error"]["code"].get<int>();

            return "⚠ WebSocket Error (" + std::to_string(errorCode) + "): " + errorMessage;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error handling WebSocket error: " << e.what() << std::endl;
    }

    return "⚠ Unknown WebSocket error";
}

bool CParser::isValidJson(const std::string& rawMessage) {
    try {
        json::parse(rawMessage);
        return true;
    } catch (...) {
        return false;
    }
}