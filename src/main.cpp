#include <iostream>
#include <string>

#include "net/server.hpp"
#include "util/dot_env.hpp"

using std::stoi;
using std::cerr;

int main(int argc, char** argv) {
    int port    = 4001;
    int threads = 4;

    try {
        auto p = getValue("PORT");
        auto t = getValue("THREADS");
        if (!p.empty()) port    = stoi(p);
        if (!t.empty()) threads = stoi(t);
    } catch (const std::exception& e) {
        cerr << "[SERVER] : Bad config: " << e.what() << "\n";
        return 1;
    }
                
    Server server(port, threads);

    auto& r = server.get_router();

    r.GET(
        "/", 
        [] (
            const http_request& req,
                  http_response& res,
            const route_parameters&
        ) {
            res.status_code = 200;
            res.status_txt  = "OK";
            res.body        = "<h1>Hello from Server++ !</h1>";
            
            res.headers["Content-Type"] = "text/html";
        }
    );

    // Explicit OPTIONS handler with CORS headers
    r.OPTIONS(
        "/api/data",
        [](const http_request&, http_response& res, const route_parameters&) {
            res.status_code = 200;
            res.status_txt  = "OK";
            res.headers["Allow"]                        = "GET, POST, OPTIONS";
            res.headers["Access-Control-Allow-Origin"]  = "*";
            res.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
            res.body = "";
        }
    );

    // GET /api/data — auto-introspection will also cover OPTIONS for this path
    r.GET(
        "/api/data",
        [](const http_request&, http_response& res, const route_parameters&) {
            res.status_code = 200;
            res.status_txt  = "OK";
            res.body        = "{\"msg\":\"hello\"}";
            res.headers["Content-Type"] = "application/json";
        }
    );

    server.start();

    return 0;
}