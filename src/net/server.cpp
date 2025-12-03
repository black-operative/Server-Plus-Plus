#include <csignal>
#include <iostream>

#include <sys/socket.h>
#include <unistd.h>

#include "net/server.hpp"

#define BUFFER_SIZE 65536

using std::endl;
using std::cout;
using std::cerr;

static Server* global_server_ptr = nullptr;

// Ctrl+C graceful shutdown handler
static void handle_sigint(int) {
    if (global_server_ptr)
        global_server_ptr->stop();
}

// Constructor
Server::Server(int port) {
    this->port      = port;
    this->running   = false;
    this->server_fd = -1;

    global_server_ptr = this;

    server_fd = socket(
        AF_INET, 
        SOCK_STREAM, 
        0
    );
    if (server_fd < 0) {
        cerr << "\n[SERVER] : socket() failed\n";
        return;
    }

    int enable = 1;
    if (
        setsockopt(
            server_fd, 
            SOL_SOCKET, 
            SO_REUSEADDR, 
            &enable, 
            sizeof(enable)
        ) < 0
    ) {
        cerr << "\n[SERVER] : setsockopt() failed\n";
        close(server_fd);
        return;
    }

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(this->port);
    
    if (
        bind(
            server_fd, 
            (struct sockaddr*) &server_addr, 
            sizeof(server_addr)
        ) < 0
    ) {
        cerr << "\n[SERVER] : bind() failed\n";
        close(server_fd);
        return;
    }
}

// Start server
bool Server::start() {
    if (server_fd < 0) {
        cerr << "[SERVER] : start() -> Cannot start — invalid FD\n";
        return false;
    }

    if (listen(server_fd, 128) < 0) {
        cerr << "[SERVER] : listen() failed\n";
        return false;
    }

    cout << "[SERVER] : Listening on port " << this->port << endl;

    signal(SIGINT, handle_sigint);

    running = true;
    accept_loop();

    return true;
}

// Stop server
void Server::stop() {
    running = false;
    close(server_fd);
    cout << "\n[SERVER] : Shutdown complete.\n";
}

// Accept loop
void Server::accept_loop() {
    while (running) {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int client_fd = accept(
            server_fd, 
            (sockaddr*) &client_addr, 
            &len
        );

        if (client_fd < 0) {
            if (running) 
                cerr << "[SERVER] accept() failed\n";
            continue;
        }

        char c_addr[INET_ADDRSTRLEN];
        inet_ntop(
            AF_INET, 
            &(client_addr.sin_addr), 
            c_addr,
            INET_ADDRSTRLEN
        );
        cout << "\n[SERVER] : Connection accepted : " << c_addr << endl;

        handle_client(client_fd);
        close(client_fd);
    }
}

// Handle one client request
void Server::handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0)
        return;

    string raw(buffer, bytes);

    http_request req;
    req.parse(raw);

    http_response res;
    res.version = "HTTP/1.1";

    router.handle(req, res);

    string out = res.toString();
    send(client_fd, out.c_str(), out.size(), 0);
}