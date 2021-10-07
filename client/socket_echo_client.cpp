#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <thread>

#include "socket_processor.h"

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

int main(int argc, char **argv) {
    // Check arguments
    if (argc < 11) {
        std::cerr << "[ERROR] Parameters are not valid" << std::endl;
        return -1;
    }

    const char *host = checkNotNull(getCmdOption(argv, argv + argc, "-h"));
    const short port = static_cast<short>(std::strtol(checkNotNull(getCmdOption(argv, argv + argc, "-p")), nullptr, 10));
    const long iterations = std::strtol(checkNotNull(getCmdOption(argv, argv + argc, "-i")), nullptr, 10);
    const long rateHz = std::strtol(checkNotNull(getCmdOption(argv, argv + argc, "-r")), nullptr, 10);
    const long payload_size = std::strtol(checkNotNull(getCmdOption(argv, argv + argc, "-s")), nullptr, 10);

    socket_processor *processor = new socket_processor(host, port, iterations, payload_size);

    const int current_cpu = sched_getcpu();
    std::thread recv_thread = std::thread(&socket_processor::receive, processor);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(current_cpu+1, &cpuset);
    int rc = pthread_setaffinity_np(recv_thread.native_handle(),
                                    sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << std::endl;
    }

    processor->sender(rateHz);

    recv_thread.join();

    delete processor;
    return 0;
}