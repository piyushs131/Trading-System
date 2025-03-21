
# *Trading System - High-Performance Order Execution and Management*
This project is a *high-performance order execution and management system* for trading on *Deribit Testnet* using *C++. It supports **real-time market data streaming, order placement, modification, and cancellation, along with **latency benchmarking and optimization*.

---

## *📌 Features*
### *1. Order Management*
✔ *Place Orders* – Supports *Market, Limit, Stop-Limit* orders.  
✔ *Modify Orders* – Adjust price and quantity for open orders.  
✔ *Cancel Orders* – Cancel an order before execution.  
✔ *View Open Orders* – Fetch all currently open orders.  
✔ *Order Book Retrieval* – Get real-time order book data.  
✔ *View Position* – Monitor open trading positions.

### *2. Real-Time Market Data (WebSocket)*
✔ *WebSocket Server* – Implements a custom WebSocket server.  
✔ *Subscription Handling* – Clients can subscribe to market data streams.  
✔ *Efficient Message Broadcasting* – Low-latency streaming to multiple clients.  

### *3. Performance Optimization*
✔ *Multi-Threaded Execution* – Uses C++ threads to parallelize operations.  
✔ *In-Memory Storage* – No external DB to reduce latency.  
✔ *Efficient Data Structures* – Uses optimized containers for fast lookup.  
✔ *Boost Asio for Networking* – Ensures low-latency socket communication.  

### *4. Latency Benchmarking & Optimization*
✔ *Measure API Response Latency* (Order execution, WebSocket updates).  
✔ *Market Data Processing Speed* (How fast new data is processed).  
✔ *WebSocket Message Propagation Delay* (End-to-end latency tracking).  
✔ *CPU & Memory Optimization* (Efficient thread scheduling).  

---

## *🚀 Getting Started*
### *1. Prerequisites*
- *Windows (MinGW) / Linux*
- *C++ Compiler (GCC/Clang/MSVC)*
- **Boost Asio (vcpkg install boost-asio)**
- **OpenSSL (vcpkg install openssl)**
- **WebSocket++ (vcpkg install websocketpp)**
- **CMake (sudo apt install cmake / Windows: choco install cmake)**

### *2. Clone Repository*
sh
git clone https://github.com/piyushs131/Trading-System.git
cd Trading-System


### *3. Configure API Keys*
Set your *Deribit API Keys* in main.cpp:
cpp
std::string client_id = "YOUR_CLIENT_ID";    
std::string client_secret = "YOUR_CLIENT_SECRET";


### *4. Build and Compile*
sh
mkdir build
cd build
cmake ..
make -j4


For *Windows (MinGW)*:
sh
g++ -std=c++11 -o trading_system.exe src/main.cpp -lssl -lcrypto -lboost_system -lpthread


### *5. Run the Application*
sh
./trading_system


---

## *🛠 Running Tests*
The system includes *latency and throughput benchmarks*.

### *1. Test Order Latency*
sh
./test/test_latency/test_latency

### *2. Test Throughput*
sh
./test/test_throughput/test_throughput

### *3. Debugging*
To enable debugging logs, set:
cpp
#define DEBUG_MODE 1

Then recompile and run the application.

---

## *📝 Documentation*
Check the **documentation/** folder for:
- *System Architecture*
- *Benchmarking Analysis*
- *Optimization Report*
- *Performance Charts*

---

## *📊 Performance Analysis*
✔ *Order Execution Latency*: ~1-2ms  
✔ *Market Data Processing Speed*: ~5,000 messages/sec  
✔ *WebSocket Update Latency*: ~500µs  
✔ *Optimized Memory Usage*: <30MB  

---

## *🔧 Future Improvements*
✅ *More Trading Strategies* – Implement AI-driven order execution.  
✅ *Multi-Exchange Support* – Extend support to Binance, Coinbase.  
✅ *Distributed Execution* – Allow multi-server deployments.  
✅ *Further Latency Reduction* – Kernel bypass networking (DPDK).  

---

## *📜 License*
This project is licensed under the *MIT License*.

---

## *👨‍💻 Contributors*
- Piyush Saxena


---

## *📬 Contact*
For support, reach out via:  
📧 Email: piyush13delhi@gmail.com  
🔗 GitHub Issues: [Open an Issue](https://github.com/piyushs131/Trading-System.git)

---

### *🚀 Start Trading Now!*
sh
./trading_system


---
Let me know if you need *any modifications*! 🚀