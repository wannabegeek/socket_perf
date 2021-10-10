#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <algorithm>
#include <thread>
#include <cmath>

#include "socket_processor.h"
#include "../../utils/tflogger.h"
#include "../../utils/tfthread_utils.h"

socket_processor::socket_processor(const char *host, const short port, const long iterations, const long payload_size) : iterations(iterations), payload_size(payload_size) {
    // Create a socket & get the file descriptor
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    // Check If the socket is created
    if (sock_fd < 0) {
        ERROR_LOG("Socket cannot be created: " << strerror(errno));
        exit(-2);
    }

    int flag = 1;
    if (setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&flag), sizeof(int)) < 0) {
        ERROR_LOG("Socket failed to set TCP_NODELAY: " << strerror(errno));
        exit(-2);
    }

    INFO_LOG("Socket has been created.");

    // Get host information by name
    // gethostbyname is not thread-safe, checkout getaddrinfo
    sockaddr_in server_addr{};
    const hostent *server = gethostbyname(host);
    if (!server) {
        ERROR_LOG("No such host");
        exit(-3);
    }
    INFO_LOG("Hostname is found.");

    // Fill address fields before try to connect
    std::memset(reinterpret_cast<char*>(&server_addr), 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Check if there is an address of the host
    if (server->h_addr_list[0])
        std::memcpy(reinterpret_cast<char*>(server->h_addr_list[0]), reinterpret_cast<char*>(&server_addr.sin_addr), server->h_length);
    else {
        ERROR_LOG("There is no a valid address for that hostname");
        exit(-5);
    }

    if (connect(sock_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        ERROR_LOG("Connection cannot be established");
        exit(-6);
    }
    INFO_LOG("Connection established.");
}


socket_processor::~socket_processor() {
    close(sock_fd);
    INFO_LOG("Socket is closed.");
}

void socket_processor::receive() const {
    INFO_LOG("Receiver running on thread: " << tf::thread::get_current_core());

    char recv_buf[4096];
    std::memset(recv_buf, 0, payload_size);

//    long *results = new long[iterations];
    std::vector<long> results;
    int index = 0;
    long *timestamp;

    do {
        // Wait for a message
        int bytes_recv = recv(sock_fd, recv_buf, payload_size, 0);
        auto now = std::chrono::high_resolution_clock::now();

        // Check how many bytes received
        // If something gone wrong
        if (bytes_recv < 0) {
            ERROR_LOG("Message cannot be received: " << strerror(errno));
        }
            // If there is no data, it means server is disconnected
        else if (bytes_recv == 0) {
            INFO_LOG("Server is disconnected." << strerror(errno));
#ifdef DEBUG
        } else {
            INFO_LOG("server> " << std::string(recv_buf, 0, bytes_recv) << strerror(errno));
#endif
        }

        char *x = recv_buf + 8;
        timestamp = reinterpret_cast<long *>(x);
        std::chrono::nanoseconds start_time_ns(*timestamp);
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time(start_time_ns);
        auto elapsed = now - start_time;

        results.push_back(elapsed.count());
    } while (index++ < iterations);

    INFO_LOG("RESULTS");
    std::cout << "25th:      " << percentile(results, 25.0) << std::endl;
    std::cout << "50th:      " << percentile(results, 50.0) << std::endl;
    std::cout << "75th:      " << percentile(results, 75.0) << std::endl;
    std::cout << "90th:      " << percentile(results, 90.0) << std::endl;
    std::cout << "95th:      " << percentile(results, 95.0) << std::endl;
    std::cout << "99th:      " << percentile(results, 99.0) << std::endl;
    std::cout << "99.9th:    " << percentile(results, 99.9) << std::endl;
    std::cout << "99.99th:   " << percentile(results, 99.99) << std::endl;

//    delete[] recv_buf;
}

long socket_processor::percentile(std::vector<long> latencies, double percentile) {
    std::sort(latencies.begin(), latencies.end());
    int index = static_cast<int>(floor(percentile / 100.0 * latencies.size()));
    return latencies[index];
}

void socket_processor::busyWaitUntil(std::chrono::time_point<std::chrono::high_resolution_clock> time) {
    while (std::chrono::high_resolution_clock::now() < time) {
        __builtin_ia32_pause();
    }
}

void socket_processor::sender(const long rateHz) const {
    std::cout << "Sender running on thread: " << tf::thread::get_current_core() << std::endl;

    char *send_buf = new char[payload_size];
    // TODO: fill this with random bytes
    std::memset(send_buf, 0, payload_size);
    long *x = reinterpret_cast<long *>(send_buf);
    *x = payload_size;

    auto start = std::chrono::high_resolution_clock::now();

    auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) / rateHz;
    int index = 0;
    do {
        auto now = std::chrono::high_resolution_clock::now();
        auto now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
        long timestamp = now_ns.time_since_epoch().count();

        long *ts = reinterpret_cast<long *>(send_buf + 8);
        *ts = timestamp;

        int bytes_send = send(sock_fd, send_buf, static_cast<size_t>(payload_size), 0);
        // Check if message sending is successful
        if (bytes_send < 0) {
            ERROR_LOG("Message cannot be sent: " << strerror(errno));
            break;
        }

        start += period;
        busyWaitUntil(start);
    } while (index++ < iterations);

    delete[] send_buf;
}
