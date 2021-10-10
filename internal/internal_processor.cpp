#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <thread>
#include <cmath>
#include <algorithm>
#include <iomanip>

#include "internal_processor.h"
#include "../utils/tflogger.h"
#include "../utils/tfthread_utils.h"

internal_processor::internal_processor(const long iterations, const long payload_size) : iterations(iterations), payload_size(payload_size) {
}


internal_processor::~internal_processor() {
    INFO_LOG("Shutting down");
}

void internal_processor::receive() {
    INFO_LOG("Receiver running on thread: " << tf::thread::get_current_core());

    char *recv_buf = nullptr;
    std::vector<long> results;
    int index = 0;
    long *timestamp;

    do {
        // Wait for a message
        while (!ringbuffer.pop(recv_buf)) {
            __builtin_ia32_pause();
        }
        auto now = std::chrono::high_resolution_clock::now();

        char *x = recv_buf + 8;
        timestamp = reinterpret_cast<long *>(x);
        std::chrono::nanoseconds start_time_ns(*timestamp);
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time(start_time_ns);
        auto elapsed = now - start_time;

        results.push_back(elapsed.count());
        delete[] recv_buf;
    } while (index++ < iterations);

    INFO_LOG("RESULTS");;
    std::cout << "\t25th:      " << std::setw(10) << percentile(results, 25.0) << std::endl;
    std::cout << "\t50th:      " << std::setw(10) << percentile(results, 50.0) << std::endl;
    std::cout << "\t75th:      " << std::setw(10) << percentile(results, 75.0) << std::endl;
    std::cout << "\t90th:      " << std::setw(10) << percentile(results, 90.0) << std::endl;
    std::cout << "\t95th:      " << std::setw(10) << percentile(results, 95.0) << std::endl;
    std::cout << "\t99th:      " << std::setw(10) << percentile(results, 99.0) << std::endl;
    std::cout << "\t99.9th:    " << std::setw(10) << percentile(results, 99.9) << std::endl;
    std::cout << "\t99.99th:   " << std::setw(10) << percentile(results, 99.99) << std::endl;
}

long internal_processor::percentile(std::vector<long> latencies, double percentile) {
    std::sort(latencies.begin(), latencies.end());
    int index = static_cast<int>(floor(percentile / 100.0 * latencies.size()));
    return latencies[index];
}

void internal_processor::busyWaitUntil(std::chrono::time_point<std::chrono::high_resolution_clock> time) {
    while (std::chrono::high_resolution_clock::now() < time) {
        __builtin_ia32_pause();
    }
}

void internal_processor::sender(const long rateHz) {
    INFO_LOG("Sender running on thread: " << sched_getcpu());

//    char *send_buf = new char[payload_size];
//    // TODO: fill this with random bytes
//    std::memset(send_buf, 0, payload_size);
//    long *x = (long *)send_buf;
//    *x = payload_size;

    auto start = std::chrono::high_resolution_clock::now();

    auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / rateHz;
    int index = 0;
    do {
        auto now = std::chrono::high_resolution_clock::now();
        auto now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
        long timestamp = now_ns.time_since_epoch().count();

        // TODO: maybe don't do this each time?
        char *send_buf = new char[payload_size];
        long *x = reinterpret_cast<long *>(send_buf);
        *x = payload_size;

        long *ts = reinterpret_cast<long *>(send_buf + 8);
        *ts = timestamp;

        if (!ringbuffer.push(send_buf)) {
            ERROR_LOG("Message cannot be sent: ring buffer full");
            break;
        }

        start += period;
        busyWaitUntil(start);
    } while (index++ < iterations);

//    delete[] send_buf;
}
