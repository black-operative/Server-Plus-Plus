# Server++

[![C++20](https://img.shields.io/badge/C%2B%2B-20-brightgreen.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.15%2B-blue.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)
[![Build](https://img.shields.io/badge/build-local-lightgrey.svg)](#)

```plaintext
===============================================================================================
  ____                           ____           _                 ____   ___  _                
 ╱ ___│  ___ _ ____   _____ _ __│  _ ╲ ___  ___│ │_ __ _ _ __ ___╱ ___│ ╱ _ ╲│ │       _     _   
 ╲___ ╲ ╱ _ ╲ '__╲ ╲ ╱ ╱ _ ╲ '__│ │_) ╱ _ ╲╱ __│ __╱ _` │ '__╱ _ ╲___ ╲│ │ │ │ │     _│ │_ _│ │_ 
  ___) │  __╱ │   ╲ V ╱  __╱ │  │  __╱ (_) ╲__ ╲ ││ (_│ │ │ │  __╱___) │ │_│ │ │___ │_   _│_   _│
 │____╱ ╲___│_│    ╲_╱ ╲___│_│  │_│   ╲___╱│___╱╲__╲__, │_│  ╲___│____╱ ╲__╲_╲_____│  │_│   │_│  
                                                   │___╱                                       
===============================================================================================
```

Server++ — A Minimal, demo-ready HTTP server framework written in modern C++ (C++20). Provides routing, middleware support, a PostgreSQL connection pool, and a small `.env` config reader. Great for demos showing a C++ REST backend without heavy dependencies.

---

## Quick facts

- Language: **C++20**
- Build: **CMake**
- Focus: Learning / Demo / Small local services
- License: **MIT**

---

## Features

- Lightweight router with parameterized routes and wildcard support
  - `GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `OPTIONS`
  - Path params (e.g. `/users/:id`) and simple wildcard (`/static/*`)
- Middleware support (pre-route hooks that can short-circuit)
- Simple `http_request` / `http_response` types
- Concurrent multi-threaded TCP server using a C++20 `std::jthread` pool (configurable worker count)
- PostgreSQL connection pool (`DB_Pool`) backed by `libpqxx`
  - RAII connection handle (`DB_Connection`) — auto-returns to the pool on destruction
  - Read-write transactions (`work`), read-only transactions (`readOnly`), non-transactional execution (`execNonTx`)
  - Prepared statement registration at pool construction and per-connection (`prepare` / `execPrepared`)
  - Configurable min/max connections, acquire timeout, and idle timeout
  - Thread-safe: multiple threads may call `acquire()` concurrently
- `.env` value reader (`getValue`) for small config
- Small surface area — easy to wire into DAOs and business logic

---

## Badges & Quick Links

- C++20, CMake build
- Local build tested on Linux/macOS/Windows (WSL)
- Example endpoints included in `src/main.cpp`

---

## Files of interest

- `include/net/server.hpp` + `src/net/server.cpp` — TCP server + accept loop
- `include/thread/thread_pool.hpp` + `src/thread/thread_pool.cpp` - C++20 jthread worker pool for concurrent client handling
- `include/net/router.hpp` + `src/net/router.cpp` — router + middleware
- `include/net/route.hpp` + `src/net/route.cpp` — route matching & params
- `include/net/http_types.hpp` + `src/net/http_types.cpp` — request/response
- `include/util/dot_env.hpp` + `src/util/dot_env.cpp` — `.env` reader
- `include/db/internal/db_config.hpp` — `DB_Config` struct (connection string, pool sizing, timeouts, prepared statements)
- `include/db/internal/db_pool.hpp` + `src/db/internal/db_pool.cpp` — thread-safe `DB_Pool`
- `include/db/internal/db_conn.hpp` + `src/db/internal/db_conn.cpp` — RAII `DB_Connection` handle
- `src/main.cpp` — minimal demo wiring

---

## Quick start

### Create `.env` in project root

```ini
# .env
# Creating this .env file is optional to configure the Server itself as the values listed below are default for the server
# .env is encourage for DB and other configs
PORT=4001
THREADS=4

# PostgreSQL — pass the full libpq connection string
DATABASE_URL=postgresql://user:password@localhost:5432/mydb
```

### Build

```bash
mkdir -p build
cmake -S . -B build
cmake --build build --target Server++
```

### Run

```bash
./build/Server++
```

### Try with curl

```bash
curl http://localhost:4001/
```

## Example usage (copy/paste)

`Note:` Example uses nlohmann/json for convenience. Add it to your project via `find_package(nlohmann_json REQUIRED)` or a single-header copy.
The database layer requires **libpqxx** (PostgreSQL C++ client). Install it via your package manager (e.g. `apt install libpqxx-dev`) and add `find_package(libpqxx REQUIRED)` + `target_link_libraries(... libpqxx::pqxx)` to your `CMakeLists.txt`.

```cpp
#include <iostream>

#include "net/server.hpp"
#include "util/dot_env.hpp"
#include "util/json.hpp"        // <- This can be copy pasted from neils nlohmann's json repository's single_include directory
#include "db/internal/db_pool.hpp"
#include "db/internal/db_config.hpp"

using json = nlohmann::json;

int main() {
    // Read port from .env or use 4001
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

    // Set up the PostgreSQL connection pool
    DB_Config dbCfg;
    dbCfg.connString     = getValue("DATABASE_URL");
    dbCfg.minConnections = 2;
    dbCfg.maxConnections = 10;
    dbCfg.preparedStatements = {
        { "get_user", "SELECT id, name FROM users WHERE id = $1" }
    };
    DB_Pool pool(dbCfg);

    Server server(port, threads);
    auto& r = server.get_router();

    // Logging middleware
    r.use(
        [] (
            const http_request& req, 
            http_response& res
        ) -> bool {
            std::cout << req.method << " " << req.path << std::endl;
            return true; 
        }
    );

    // Simple GET
    r.GET(
        "/", 
        [] (
            const http_request& req, 
            http_response& res, 
            const route_parameters& params
        ) {
            res.version = "HTTP/1.1";
            res.status_code = 200;
            res.status_txt  = "OK";
            
            res.body = "Hello from Server++";

            res.headers["Content-Type"] = "text/plain";
        }
    );

    // Query via the pool — connection is automatically returned on scope exit
    r.GET(
        "/users/:id",
        [&pool] (
            const http_request& req,
            http_response& res,
            const route_parameters& params
        ) {
            auto conn = pool.acquire();
            auto rows = conn.execPrepared("get_user", std::stoi(params.at("id")));

            json out;
            if (rows.empty()) {
                res.status_code = 404;
                res.status_txt  = "Not Found";
                out["error"] = "User not found";
            } else {
                res.status_code = 200;
                res.status_txt  = "OK";
                out["id"]   = rows[0]["id"].as<int>();
                out["name"] = rows[0]["name"].as<std::string>();
            }

            res.version = "HTTP/1.1";
            res.body    = out.dump(4);
            res.headers["Content-Type"] = "application/json";
        }
    );

    // Echo JSON POST
    r.POST(
        "/api/data", 
        [] (
            const http_request& req, 
            http_response& res, 
            const route_parameters& params
        ) {
            json out;
            out["msg"] = "POST Received";

            try {
                out["req"] = json::parse(req.body);
            } catch (...) {
                out["req"] = nullptr;
                out["error"] = "Invalid JSON";
            }
            
            res.version = "HTTP/1.1";
            res.status_code = 201;
            res.status_txt  = "Created";
            
            res.body = out.dump(4);
            
            res.headers["Content-Type"] = "application/json";
        }
    );

    server.start();
    return 0;
}
```

## API Reference (essential)

### Server

- `Server::Server(int port, size_t thread_count = 4)` — initializes server and allocates worker threads
- `Server::start()` — start accept loop, blocking
- `Server::stop()` — stops server, closes socket
- `Server::get_router()` — reference to Router to register routes & middleware

### Router

- `GET(path, handler)`, `POST(path, handler)`, `PUT`, `PATCH`, `DELETE`
- `use(middleware)` — register middleware: bool(const http_request&, http_response&)
- `handle(req, res)` — internal : run middleware + matched route

### http_request

- `method`, `path`, `version`, `headers (map)`, `body`
- `parse(raw_request_string)` method provided

### http_response

- `version`, `status_code`, `status_txt`, `headers (map)`, `body`
- `toString()` to get full wire representation

### DB_Config

```cpp
struct DB_Config {
    string       connString;                   // libpq connection string
    size_t       minConnections = 1;           // connections kept alive when idle
    size_t       maxConnections = 10;          // hard cap on open connections
    milliseconds acquireTimeout{5000};         // how long acquire() blocks before throwing
    seconds      idleTimeout   {300};          // evict idle connections older than this

    struct PreparedStatement { string name, sql; };
    vector<PreparedStatement> preparedStatements; // registered on every new connection
};
```

### DB_Pool

- `DB_Pool(DB_Config cfg)` — opens `maxConnections` connections eagerly; registers all prepared statements
- `DB_Connection acquire()` — blocks up to `acquireTimeout`; evicts stale/dead connections; throws on timeout
- `Stats stats() const` — returns `{ total, idle, inUse }` counts
- `void reset()` — drains idle queue, re-opens `minConnections` fresh connections
- `void release(unique_ptr<connection>)` — called automatically by `DB_Connection` destructor

### DB_Connection

RAII handle returned by `DB_Pool::acquire()`. Returns the connection to the pool on destruction. **Not copyable; move-only.**

| Method | Description |
|---|---|
| `work(fn)` | Runs `fn(pqxx::work&)`, commits on success, re-throws on exception |
| `readOnly(fn)` | Runs `fn(pqxx::read_transaction&)` — no writes allowed |
| `execNonTx(sql)` | Executes SQL outside a transaction (DDL, `SET`, `COPY`, …) |
| `prepare(name, sql)` | Prepares a named statement on this connection |
| `execPrepared(name, args…)` | Executes a prepared statement in an auto-committed transaction |
| `isAlive()` | Pings the server with `SELECT 1`; returns `false` on any error |
| `raw()` | Direct access to the underlying `pqxx::connection` |

## Routes & Parameter Examples

| Route            | Example request        | params           |
| ---------------- | ---------------------- | ---------------- |
| `GET /users/:id` | `/users/42`            | `{ "id": "42" }` |
| `GET /static/*`  | `/static/css/main.css` | wildcard match   |
| `POST /api/data` | JSON body accepted     | none             |

## Architecture

![Architecture](assets/Architecture.svg)

## Request → Router → Handler flowchart

![Flowchart](assets/Flowchart.svg)

## License

MIT see [LICENSE](LICENSE)
