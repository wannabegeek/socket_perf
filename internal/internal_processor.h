#ifndef SOCKET_PERF_SOCKET_PROCESSOR_H
#define SOCKET_PERF_SOCKET_PROCESSOR_H

#include <vector>
#include <chrono>
#include "../utils/tfringbuffer.h"

class internal_processor {
private:
    tf::ringbuffer<char *> ringbuffer;

    const long iterations;
    const long payload_size;

    static long percentile(std::vector<long> latencies, double percentile);
    static void busyWaitUntil(std::chrono::time_point<std::chrono::high_resolution_clock> time);
public:
    internal_processor(long iterations, long payload_size);
    ~internal_processor();

    void sender(long rateHz);
    void receive();
};

#endif //SOCKET_PERF_SOCKET_PROCESSOR_H
