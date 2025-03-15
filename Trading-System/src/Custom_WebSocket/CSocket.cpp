#include "../Custom_WebSocket/CSocket.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <arpa/inet.h>  // for htons

namespace CustomWebSocket {

// Helper: Base64 encode a given binary string
static std::string base64Encode(const std::string& input) {
    static const char* b64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);
    unsigned val = 0;
    int valb = -6;
    for (uint8_t c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(b64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        output.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (output.size() % 4) {
        output.push_back('=');
    }
    return output;
}

// Generate Sec-WebSocket-Accept given a Sec-WebSocket-Key
static std::string generateAcceptKey(const std::string& key) {
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string input = key + magic;
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
    std::string hashed(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH);
    return base64Encode(hashed);
}

// Convert 64-bit value from host to network byte order
static uint64_t hostToNetwork64(uint64_t val) {
    uint64_t result = 0;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&val);
    // Append bytes in reverse order (big-endian)
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) | bytes[i];
    }
    return result;
}

CSocket::CSocket()
    : tcpSocket(io), sslContext(boost::asio::ssl::context::tlsv12_client),
      sslStream(tcpSocket, sslContext), running(false) {
    sslContext.set_verify_mode(boost::asio::ssl::verify_none);
}

CSocket::~CSocket() {
    close();
}

bool CSocket::connect(const std::string& host, const std::string& path) {
    try {
        // Resolve host address
        boost::asio::ip::tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(host, "443");
        boost::asio::connect(tcpSocket, endpoints);
        // TLS handshake
        sslStream.handshake(boost::asio::ssl::stream_base::client);

        // Generate Sec-WebSocket-Key (16 random bytes base64)
        unsigned char randKey[16];
        RAND_bytes(randKey, 16);
        std::string secKey = base64Encode(std::string(reinterpret_cast<char*>(randKey), 16));

        // Build WebSocket handshake request
        std::ostringstream req;
        req << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << "Sec-WebSocket-Key: " << secKey << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n"
            << "\r\n";
        std::string request = req.str();
        boost::asio::write(sslStream, boost::asio::buffer(request));

        // Read handshake response
        boost::asio::streambuf responseBuf;
        boost::asio::read_until(sslStream, responseBuf, "\r\n\r\n");
        std::istream responseStream(&responseBuf);
        std::string responseLine;
        std::getline(responseStream, responseLine);
        if (responseLine.find("101") == std::string::npos) {
            std::cerr << "WebSocket handshake failed: " << responseLine << std::endl;
            return false;
        }
        std::string header;
        std::string acceptKey;
        while (std::getline(responseStream, header) && header != "\r") {
            if (header.rfind("Sec-WebSocket-Accept:", 0) == 0) {
                acceptKey = header.substr(header.find(":") + 2);
                // Remove any trailing CR or LF
                if (!acceptKey.empty() && acceptKey.back() == '\r') {
                    acceptKey.pop_back();
                }
            }
        }
        std::string expectedAccept = generateAcceptKey(secKey);
        if (acceptKey != expectedAccept) {
            std::cerr << "Sec-WebSocket-Accept mismatch, expected " << expectedAccept 
                      << ", got " << acceptKey << std::endl;
            return false;
        }
        // WebSocket handshake successful
        running = true;
        // Subscribe to order book channel (e.g., top 10 levels, 100ms interval)
        std::string subscribeMsg = 
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"public/subscribe\",\"params\":{\"channels\":[\"book.BTC-PERPETUAL.none.10.100ms\"]}}";
        send(subscribeMsg);
        // Start reader thread
        recvThread = std::thread(&CSocket::readerLoop, this);
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "CSocket connect error: " << ex.what() << std::endl;
        return false;
    }
}

bool CSocket::send(const std::string& message) {
    if (!running) return false;
    std::lock_guard<std::mutex> lock(sendMutex);
    // Prepare WebSocket frame for client-to-server message
    uint8_t header[14]; // maximum header size for 64-bit length
    size_t headerLen = 0;
    header[0] = 0x81; // FIN=1, opcode=1 (text)
    // Determine payload length
    size_t payloadLen = message.size();
    if (payloadLen <= 125) {
        header[1] = static_cast<uint8_t>(payloadLen) | 0x80;
        headerLen = 2;
    } else if (payloadLen <= 0xFFFF) {
        header[1] = 126 | 0x80;
        uint16_t lenN = htons(static_cast<uint16_t>(payloadLen));
        std::memcpy(header + 2, &lenN, 2);
        headerLen = 4;
    } else {
        header[1] = 127 | 0x80;
        uint64_t lenN = hostToNetwork64(static_cast<uint64_t>(payloadLen));
        std::memcpy(header + 2, &lenN, 8);
        headerLen = 10;
    }
    // Generate random mask
    uint8_t mask[4];
    RAND_bytes(mask, 4);
    std::memcpy(header + headerLen, mask, 4);
    headerLen += 4;
    // Mask the payload
    std::string maskedPayload = message;
    for (size_t i = 0; i < payloadLen; ++i) {
        maskedPayload[i] ^= mask[i % 4];
    }
    try {
        boost::asio::write(sslStream, boost::asio::buffer(header, headerLen));
        if (payloadLen > 0) {
            boost::asio::write(sslStream, boost::asio::buffer(maskedPayload));
        }
    } catch (const std::exception& ex) {
        std::cerr << "CSocket send error: " << ex.what() << std::endl;
        return false;
    }
    return true;
}

void CSocket::readerLoop() {
    try {
        while (running) {
            // Read frame header (at least 2 bytes)
            uint8_t hdr[2];
            boost::asio::read(sslStream, boost::asio::buffer(hdr, 2));
            uint8_t byte1 = hdr[0];
            uint8_t byte2 = hdr[1];
            bool fin = (byte1 & 0x80) != 0;
            uint8_t opcode = byte1 & 0x0F;
            bool mask = (byte2 & 0x80) != 0;
            uint64_t payloadLen = byte2 & 0x7F;
            if (payloadLen == 126) {
                uint8_t ext[2];
                boost::asio::read(sslStream, boost::asio::buffer(ext, 2));
                payloadLen = (ext[0] << 8) | ext[1];
            } else if (payloadLen == 127) {
                uint8_t ext[8];
                boost::asio::read(sslStream, boost::asio::buffer(ext, 8));
                payloadLen = 0;
                for (int i = 0; i < 8; ++i) {
                    payloadLen = (payloadLen << 8) | ext[i];
                }
            }
            uint8_t maskKey[4] = {0,0,0,0};
            if (mask) {
                // Read mask (though server frames should not be masked)
                boost::asio::read(sslStream, boost::asio::buffer(maskKey, 4));
            }
            if (opcode == 0x8) { // close frame
                // Received close frame from server
                running = false;
                break;
            }
            // Read payload
            std::vector<char> payload;
            if (payloadLen > 0) {
                payload.resize(payloadLen);
                boost::asio::read(sslStream, boost::asio::buffer(payload.data(), payloadLen));
            }
            if (mask) {
                for (size_t i = 0; i < payload.size(); ++i) {
                    payload[i] ^= maskKey[i % 4];
                }
            }
            std::string message(payload.begin(), payload.end());
            if (message.find("\"channel\":\"book.") != std::string::npos) {
                parser.parseOrderBookUpdate(message);
                // Example: output best bid/ask for verification
                OrderBook ob = parser.getOrderBook();
                if (!ob.bids.empty() && !ob.asks.empty()) {
                    double bestBid = ob.bids.begin()->first;
                    double bestAsk = ob.asks.begin()->first;
                    std::cout << "[CustomWS] BestBid=" << bestBid 
                              << ", BestAsk=" << bestAsk << std::endl;
                }
            } else {
                // Print other messages (e.g., subscription ack or heartbeats)
                std::cout << "[CustomWS] Received: " << message << std::endl;
            }
        }
    } catch (const std::exception& ex) {
        if (running) {
            std::cerr << "Reader loop exception: " << ex.what() << std::endl;
        }
        running = false;
    }
}

void CSocket::close() {
    if (!running) {
        if (recvThread.joinable()) recvThread.join();
        return;
    }
    running = false;
    try {
        // Send a close frame to server
        uint8_t closeFrame[6];
        closeFrame[0] = 0x88; // FIN + opcode 8 (Close)
        closeFrame[1] = 0x80; // Masked, length=0
        uint8_t mask[4];
        RAND_bytes(mask, 4);
        std::memcpy(closeFrame+2, mask, 4);
        boost::asio::write(sslStream, boost::asio::buffer(closeFrame, 6));
    } catch (...) {
        // ignore send errors on close
    }
    try { sslStream.shutdown(); } catch (...) {}
    try { tcpSocket.close(); } catch (...) {}
    if (recvThread.joinable()) {
        recvThread.join();
    }
}

} // namespace CustomWebSocket