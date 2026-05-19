#include <mutex>
#include <chrono>
#include <memory>
#include <string>
#include <cstddef>
#include <utility>
#include <stdexcept>
#include <condition_variable>

#include <pqxx/pqxx>

#include "db/internal/db_pool.hpp"
#include "db/internal/db_conn.hpp"

using std::unique_lock;
using std::chrono::steady_clock;

using pqxx::nontransaction;

DB_Pool::DB_Pool(DB_Config cfg) : config(std::move(cfg)) {
    if (this->config.minConnections > this->config.maxConnections)
        throw std::invalid_argument("[Database] @ DB_Pool : minConnections > maxConenctions");

    for (size_t i = 0; i < this->config.maxConnections; i++) {
        auto c = openConnection();
        this->idle.push(
            {
                std::move(c),
                steady_clock::now()
            }
        );
        this->total++;
    }
}

DB_Pool::~DB_Pool() {
    unique_lock lock(this->mtx);
    while (!this->idle.empty()) {
        this->idle.pop();
    }
}

DB_Connection DB_Pool::acquire() {
    unique_lock lock(this->mtx);
    
    const auto deadline = steady_clock::now() + this->config.acquireTimeout;

    while (true) {
        // 1. Remove and return from idle queue 
        while (!this->idle.empty()) {
            auto entry = std::move(this->idle.front());
            this->idle.pop();

            // Boot the connection if it's overstaying and not alone
            auto age = steady_clock::now() - entry.last_used;
            if (age > this->config.idleTimeout && total > this->config.minConnections) {
                this->total--;
                continue;
            }

            // Boot the connection if it's dead
            if (!isConnectionAilve(*entry.conn)) {
                this->total--;
                continue;
            }

            return DB_Connection(std::move(entry.conn), *this);
        }

        // 2. Nothing yet exists, create one
        if (this->total < this->config.minConnections) {
            auto c = openConnection();
            this->total++;

            return DB_Connection(std::move(c), *this);
        }

        // 3. All in use, wait for return
        if (this->cv.wait_until(lock, deadline) == std::cv_status::timeout) {
            throw std::runtime_error(
                "[Database] @ acquire : Timeout after "             +
                std::to_string(this->config.acquireTimeout.count()) + 
                "ms"
            );
        }
    }
}

DB_Pool::Stats DB_Pool::stats() const noexcept {
    unique_lock lock(this->mtx);

    size_t idle_cnt  = this->idle.size();
    size_t total_cnt = this->total.load();

    return {
        total_cnt,
        idle_cnt,
        total_cnt - idle_cnt
    };
}

void DB_Pool::reset() {
    unique_lock lock(this->mtx);

    while (!this->idle.empty()) {
        this->idle.pop();
    }

    this->total = 0;

    for (size_t i = 0; i < config.minConnections; i++) {
        auto e = openConnection();
        this->idle.push(
            {
                std::move(e),
                steady_clock::now()
            }
        );
        this->total++;
    }
}

void DB_Pool::release(unique_ptr<connection> conn) {
    if (!conn) return;
    {
        unique_lock lock(this->mtx);
        if (conn->is_open() && this->total <= this->config.maxConnections) {
            idle.push(
                {
                    std::move(conn),
                    steady_clock::now()
                }
            );
        } else {
            this->total--;
        }
    }
    this->cv.notify_one();
}

unique_ptr<connection> DB_Pool::openConnection() {
    auto c = std::make_unique<connection>(this->config.connString);
    prepareAll(*c);
    return c;
}

bool DB_Pool::isConnectionAilve(connection& c) noexcept {
    if (!c.is_open()) return false;

    try {
        nontransaction n_transaction(c);
        n_transaction.exec("SELECT 1");
        return true;
    } catch (...) {
        return false;
    }
}

void DB_Pool::prepareAll(connection& c) {
    for (auto& statement : this->config.preparedStatements) {
        c.prepare(
            statement.name, 
            statement.sql
        );
    }
}