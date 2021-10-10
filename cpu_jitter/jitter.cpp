//
// Created by tom on 10/10/2021.
//

#include <sched.h>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <thread>
#include <vector>
#include <iomanip>

#include "../utils/tflogger.h"

int main(int argc, char *argv[]) {

    auto runtime = std::chrono::seconds(5);
    auto threshold = std::chrono::nanoseconds::max();
    size_t nsamples = runtime.count() * (1 << 16);

    int opt;
    while ((opt = getopt(argc, argv, "r:s:t:")) != -1) {
        switch (opt) {
            case 'r':
                runtime = std::chrono::seconds(std::stoul(optarg));
                break;
            case 't':
                threshold = std::chrono::nanoseconds(std::stoul(optarg));
                break;
            case 's':
                nsamples = std::stoul(optarg);
                break;
            case ':':
                break;
            default:
                ERROR_LOG("usage: jitter [-r runtime_seconds] [-t threshold_nanoseconds] [-s number_of_samples]");
                exit(1);
        }
    }

    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(set), &set) == -1) {
        perror("sched_getaffinity");
        exit(1);
    }

    // enumerate available CPUs
    std::vector<int> cpus;
    for (int i = 0; i < CPU_SETSIZE; ++i) {
        if (CPU_ISSET(i, &set)) {
            cpus.push_back(i);
        }
    }

    // calculate threshold as minimum timestamp delta * 8
    if (threshold == std::chrono::nanoseconds::max()) {
        auto ts1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 10000; ++i) {
            auto ts2 = std::chrono::high_resolution_clock::now();
            if (ts2 - ts1 < threshold) {
                threshold = ts2 - ts1;
            }
            ts1 = ts2;
        }
        threshold *= 8;
    }

    std::map<int, std::vector<std::chrono::nanoseconds>> samples;
    for (int cpu : cpus) {
        samples[cpu].reserve(nsamples);
    }

    // avoid page faults and TLB shootdowns when saving samples
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        WARNING_LOG("Failed to lock memory, increase RLIMIT_MEMLOCK or run with CAP_IPC_LOC capability: " << strerror(errno));
    }

    const auto deadline = std::chrono::high_resolution_clock::now() + runtime;
    std::atomic<size_t> active_threads = {0};

    auto func = [&](int cpu) {
        // pin current thread to assigned CPU
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu, &set);
        if (sched_setaffinity(0, sizeof(set), &set) == -1) {
            ERROR_LOG("Failed to set affinity for thread: " << strerror(errno));
            exit(1);
        }

        auto &s = samples[cpu];
        active_threads.fetch_add(1, std::memory_order_release);

        // wait for all threads to be ready
        while (active_threads.load(std::memory_order_relaxed) != cpus.size());

        // run jitter measurement loop
        auto ts1 = std::chrono::high_resolution_clock::now();
        while (ts1 < deadline) {
            auto ts2 = std::chrono::high_resolution_clock::now();
            if (ts2 - ts1 < threshold) {
                ts1 = ts2;
                continue;
            }
            if (s.size() == s.capacity()) {
                ERROR_LOG("pre-allocated sample space exceeded, increase threshold or number of samples.");
            }
            s.push_back(ts2 - ts1);
            // ts1 = ts2;
            ts1 = std::chrono::high_resolution_clock::now();
        }
    };

    // start measurements threads
    std::vector<std::thread> threads;
    for (auto it = ++cpus.begin(); it != cpus.end(); ++it) {
        threads.emplace_back([&, cpu = *it] { func(cpu); });
    }
    func(cpus.front());

    // wait for all threads to finish
    for (auto &t : threads) {
        t.join();
    }

    // print statistics
    std::cout << "       cpu threshold    hiccups    50pct     99pct   99.9pct    max_ns" << std::endl;
    for (auto &[cpu, s] : samples) {
        std::sort(s.begin(), s.end());
        std::cout << std::setw(10)
                << cpu << std::setw(10) << threshold.count() << std::setw(10) << s.size() << std::setw(10)
                << s[s.size() * 0.5].count() << std::setw(10)
                << s[s.size() * 0.99].count() << std::setw(10)
                << s[s.size() * 0.999].count() << std::setw(10) << s.back().count()
                << std::endl;
    }

    return 0;
}