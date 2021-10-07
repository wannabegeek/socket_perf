//
// Created by tom on 07/10/2021.
//

#ifndef SOCKET_PERF_SOCKET_PROCESSOR_H
#define SOCKET_PERF_SOCKET_PROCESSOR_H


class socket_processor {
private:
    int sock_fd;
    const long iterations;
    const long payload_size;

    long percentile(std::vector<long> latencies, double percentile);
    void busyWaitUntil(std::chrono::time_point<std::chrono::steady_clock> time);
public:
    socket_processor(const char *host, const short port, const long iterations, const long payload_size);
    ~socket_processor();

    void sender(const long rateHz);
    void receive();
};


#endif //SOCKET_PERF_SOCKET_PROCESSOR_H
