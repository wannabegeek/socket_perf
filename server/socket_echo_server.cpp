#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cctype>
#include <cstring>

bool is_int(char *c) {
    while (*c != '\0') {
        if (!isdigit(*c)) {
            return false;
        }
        c++;
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2 || !is_int(argv[1])) {
        std::cerr << "[ERROR] Port is not provided via command line parameters." << std::endl;
        return -1;
    }

    // Create a socket & get the file descriptor
    int sock_listener = socket(AF_INET, SOCK_STREAM, 0);
    // Check If the socket is created
    if (sock_listener < 0) {
        std::cerr << "[ERROR] Socket cannot be created: " << strerror(errno) << std::endl;
        return -2;
    }

    std::cout << "[INFO] Socket has been created." << std::endl;
    // Address info to bind socket
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(std::atoi(argv[1]));
    //server_addr.sin_addr.s_addr = INADDR_ANY;
    // OR
    inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr);

    char buf[INET_ADDRSTRLEN];

    // Bind socket
    if (bind(sock_listener, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[ERROR] Created socket cannot be bound to ( "
                  << inet_ntop(AF_INET, &server_addr.sin_addr, buf, INET_ADDRSTRLEN)
                  << ":" << ntohs(server_addr.sin_port) << ")" << std::endl;
        return -3;
    }

    std::cout << "[INFO] Sock is bound to ("
              << inet_ntop(AF_INET, &server_addr.sin_addr, buf, INET_ADDRSTRLEN)
              << ":" << ntohs(server_addr.sin_port) << ")" << std::endl;


    // Start listening
    if (listen(sock_listener, SOMAXCONN) < 0) {
        std::cerr << "[ERROR] Socket cannot be switched to listen mode: " << strerror(errno) << std::endl;
        return -4;
    }
    std::cout << "[INFO] Socket is listening now." << std::endl;

    // Accept a call
    sockaddr_in client_addr{};
    socklen_t client_addr_size = sizeof(client_addr);
    int sock_client;
    if ((sock_client = accept(sock_listener, (sockaddr *) &client_addr, &client_addr_size)) < 0) {
        std::cerr << "[ERROR] Connections cannot be accepted for a reason: " << strerror(errno) << std::endl;
        return -5;
    }

    std::cout << "[INFO] A connection is accepted now." << std::endl;

    // Close main listener socket
    close(sock_listener);
    std::cout << "[INFO] Main listener socket is closed." << std::endl;

    // Get name info
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];
    if (getnameinfo((sockaddr *) &client_addr, client_addr_size,host, NI_MAXHOST,svc, NI_MAXSERV, 0) != 0) {
        std::cout << "[INFO] Client: (" << inet_ntop(AF_INET, &client_addr.sin_addr, buf, INET_ADDRSTRLEN)
                  << ":" << ntohs(client_addr.sin_port) << ")" << std::endl;
    } else {
        std::cout << "[INFO] Client: (host: " << host << ", service: " << svc << ")" << std::endl;
    }


    char msg_buf[2048];
    int bytes;
    // While receiving - display & echo msg
    while (true) {
        bytes = recv(sock_client, &msg_buf, 2048, 0);
        // Check how many bytes recieved
        // If there is no data, it means server is disconnected
        if (bytes == 0) {
            std::cout << "[INFO] Client is disconnected." << std::endl;
            break;
        }
            // If something gone wrong
        else if (bytes < 0) {
            std::cerr << "[ERROR] Something went wrong while receiving data: " << strerror(errno) << std::endl;
            break;
        }
            // If there is some bytes
        else {
#ifdef DEBUG
            // Print message
            long *msg_len = (long *)msg_buf;
            long *timestamp = (long *)(msg_buf + 8);
            std::chrono::nanoseconds start_time_ns(*timestamp);
            std::chrono::time_point<std::chrono::steady_clock> start_time(start_time_ns);
            std::cout << "client> len: " << *msg_len << " sending time: " << *timestamp << "\n";
#endif
            // Resend the same message
            if (send(sock_client, &msg_buf, bytes, 0) < 0) {
                std::cerr << "[ERROR] Message cannot be send, exiting: " << strerror(errno) << std::endl;
                break;
            }
        }

    }

    // Close client socket
    close(sock_client);
    std::cout << "[INFO] Client socket is closed." << std::endl;

    return 0;
}