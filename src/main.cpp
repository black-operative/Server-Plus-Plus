#include <iostream>
#include <memory>
#include <string>

#include "net/server.hpp"
#include "util/dot_env.hpp"
#include "db/internal/db_pool.hpp"       // PgPool, PgPoolConfig

using std::cerr;
using std::stoi;
using std::string;

int main(void) {
    // Load config from .env 
    int port    = 4001;
    int threads = 4;

    string db_Conn   = "host=localhost port=5432 dbname=mydb user=postgres password=secret";
    int    db_Min    = 2;
    int    db_Max    = 10;

    try {
        auto p  = getValue("PORT");
        auto t  = getValue("THREADS");
        auto cs = getValue("DB_CONN");      
        auto mn = getValue("DB_POOL_MIN");  
        auto mx = getValue("DB_POOL_MAX");

        if (!p.empty())  port    = stoi(p);
        if (!t.empty())  threads = stoi(t);
        if (!cs.empty()) db_Conn  = cs;
        if (!mn.empty()) db_Min   = stoi(mn);
        if (!mx.empty()) db_Max   = stoi(mx);
    } catch (const std::exception& e) {
        cerr << "[SERVER] Bad config: " << e.what() << "\n";
        return 1;
    }

    // Initialise the database
    DB_Config db_config;
    db_config.connString     = db_Conn;
    db_config.minConnections = static_cast<std::size_t>(db_Min);
    db_config.maxConnections = static_cast<std::size_t>(db_Max);
    db_config.acquireTimeout = std::chrono::milliseconds{3000};
    db_config.idleTimeout    = std::chrono::seconds{120};

    // Register any prepared statements used across all routes
    db_config.preparedStatements = {
        { "health_check", "SELECT 1" },
    };

    // The pool is shared across all request-handler threads via a shared_ptr.
    // Each handler calls pool->acquire() to borrow an exclusive connection.
    auto pool = std::make_shared<DB_Pool>(db_config);

    {
        auto s = pool->stats();
        std::cout << "[DB] Pool ready  total=" << s.total
                  << "  idle="  << s.idle << "\n";
    }

    // Build the server & routes
    Server server(port, threads);
    auto& r = server.get_router();

    // GET /
    r.GET(
        "/",
        [](const http_request&, http_response& res, const route_parameters&) {
            res.status_code = 200;
            res.status_txt  = "OK";
            res.body        = "<h1>Hello from Server++ !</h1>";
            res.headers["Content-Type"] = "text/html";
        }
    );

    // GET /api/db/health  — live round-trip to PostgreSQL
    r.GET(
        "/api/db/health",
        [pool] (
            const http_request&, 
                    http_response& res, 
            const route_parameters&
        ) {
            try {
                auto conn = pool->acquire();
                auto rows = conn.readOnly(
                    [] (pqxx::read_transaction& tx) {
                        return tx.exec("SELECT now() AS ts");
                    }
                );

                auto s = pool->stats();
                res.status_code = 200;
                res.status_txt  = "OK";
                res.body = R"({"status":"ok","server_time":")"
                            + std::string(rows[0]["ts"].c_str())
                            + R"(","pool":{"total":)" + std::to_string(s.total)
                            + R"(,"idle":")"          + std::to_string(s.idle)
                            + R"(","inUse":)"         + std::to_string(s.inUse)
                            + "}}";
                res.headers["Content-Type"] = "application/json";

            } catch (const std::exception& e) {
                res.status_code = 503;
                res.status_txt  = "Service Unavailable";
                res.body        = std::string(R"({"status":"error","detail":")") + e.what() + "\"}";
                res.headers["Content-Type"] = "application/json";
            }
        }
    );

    // GET /api/db/stats  — pool diagnostics
    r.GET(
        "/api/db/stats",
        [pool](const http_request&, http_response& res, const route_parameters&) {
            auto s = pool->stats();
            res.status_code = 200;
            res.status_txt  = "OK";
            res.body = R"({"total":)" + std::to_string(s.total)
                     + R"(,"idle":")" + std::to_string(s.idle)
                     + R"(","inUse":)" + std::to_string(s.inUse) + "}";
            res.headers["Content-Type"] = "application/json";
        }
    );

    // Start
    server.start();
    return 0;
}