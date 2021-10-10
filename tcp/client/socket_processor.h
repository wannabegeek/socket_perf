#ifndef SOCKET_PERF_SOCKET_PROCESSOR_H
#define SOCKET_PERF_SOCKET_PROCESSOR_H

#include <vector>
#include <chrono>

class socket_processor {
private:
    int sock_fd;
    const long iterations;
    const long payload_size;

    static long percentile(std::vector<long> latencies, double percentile);
    static void busyWaitUntil(std::chrono::time_point<std::chrono::high_resolution_clock> time);
public:
    socket_processor(const char *host, short port, long iterations, long payload_size);
    ~socket_processor();

    void sender(long rateHz) const;
    void receive() const;
};

#endif //SOCKET_PERF_SOCKET_PROCESSOR_H
