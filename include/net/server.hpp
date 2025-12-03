#pragma once

#include <atomic>

#include <arpa/inet.h>

#include "router.hpp"

using std::atomic;

class Server {
    private:
        int          port;    
        int          server_fd;
        sockaddr_in  server_addr;
        Router       router;
        atomic<bool> running;

        void accept_loop();
        void handle_client(int client_fd);
        
    public:
        explicit Server(int port);

        bool start(); // Start server
        void stop();  // Stop gracefully
        
        Router& get_router() { return router; }
};