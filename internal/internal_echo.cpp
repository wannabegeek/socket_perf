#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <pthread.h>
#include <sys/mman.h>

#include "internal_processor.h"
#include "../utils/tflogger.h"
#include "../utils/tfthread_utils.h"

// Note: Check for SIGPIPE and other socket related signals

bool is_int(char *c) {
    while (*c != '\0') {
        if (!isdigit(*c)) {
            return false;
        }
        c++;
    }
    return true;
}

char* getCmdOption(char ** begin, char ** end, const std::string & option) {
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return nullptr;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

const char *checkNotNull(const char *value) {
    if (value == nullptr) {
        throw std::invalid_argument("Invalid command line options");
    }

    return value;
}

void usage() {
    ERROR_LOG("usage: internal_echo [-i iterations] [-r rate (hz)] [-s payload size (default: 200b)]");
    exit(1);
}

int main(int argc, char **argv) {

    long iterations = -1;
    long rateHz = -1;
    long payload_size = 200;

    int opt;
    while ((opt = getopt(argc, argv, "i:s:r:")) != -1) {
        switch (opt) {
            case 'i':
                iterations = std::stol(optarg);
                break;
            case 'r':
                rateHz = std::stol(optarg);
                break;
            case 's':
                payload_size = std::stol(optarg);
                break;
            case ':':
                break;
            default:
                usage();
                break;
        }
    }
    if (rateHz == -1 || iterations == -1) {
        usage();
    }

    auto *processor = new internal_processor(iterations, payload_size);

    // avoid page faults and TLB shootdowns
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        WARNING_LOG("Failed to lock memory, increase RLIMIT_MEMLOCK or run with CAP_IPC_LOC capability: " << strerror(errno));
    }

    const int current_cpu = sched_getcpu();
    tf::thread::set_affinity(pthread_self(), { current_cpu });
    std::thread recv_thread = std::thread(&internal_processor::receive, processor);
    tf::thread::set_affinity(recv_thread, { current_cpu + 1 });

    processor->sender(rateHz);

    recv_thread.join();

    delete processor;
    return 0;
}