#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "Api.hpp"
#include "utility.hpp"

int main() {
    // Initialize API and perform mock authentication
    Api api;
    api.authenticate("testuser", "testpass");

    // Determine number of worker threads (use hardware concurrency or default to 4)
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    // Set workload per thread
    const int ordersPerThread = 100000;
    long long totalOrders = (long long) ordersPerThread * numThreads;

    // Worker function for each thread: place orders in a loop
    auto worker = [&](int threadId) {
        // Each order uses the same sample data (no need for unique content in throughput test)
        Order order;
        order.symbol = "TEST";
        order.side = "BUY";
        order.quantity = 1;
        order.price = 100.0;
        for (int i = 0; i < ordersPerThread; ++i) {
            api.placeOrder(order);
        }
    };

    // Launch threads and measure start time
    std::vector<std::thread> workers;
    workers.reserve(numThreads);
    auto start_time = std::chrono::high_resolution_clock::now();
    for (unsigned int t = 0; t < numThreads; ++t) {
        workers.emplace_back(worker, t);
    }
    // Wait for all threads to finish
    for (auto& th : workers) {
        th.join();
    }
    // Measure end time
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    double seconds = duration.count();
    // Compute throughput (orders per second)
    double ordersPerSec = totalOrders / seconds;

    // Log the throughput results as JSON
    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "{\"event\":\"throughput_result\""
                  << ",\"threads\":" << numThreads
                  << ",\"total_orders\":" << totalOrders
                  << ",\"duration_s\":" << seconds
                  << ",\"orders_per_sec\":" << ordersPerSec
                  << "}" << std::endl;
    }

    api.disconnect();
    return 0;
}