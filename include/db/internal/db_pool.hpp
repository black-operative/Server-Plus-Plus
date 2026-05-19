#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <vector>

#include "db_conn.hpp"
#include "db_config.hpp"

using std::queue;
using std::mutex;
using std::atomic;
using std::vector;
using std::string;
using std::unique_ptr;
using std::condition_variable;

using time_point = std::chrono::steady_clock::time_point;
using std::chrono::milliseconds;
using std::chrono::seconds;

using pqxx::connection;

/**
 * @brief Thread-safe database connection pool.
 *
 * Owns a queue of idle pqxx::connection objects. Multiple threads may call
 * acquire() concurrently; each receives an exclusive DB_Connection that
 * auto-returns to the pool on destruction.
 */
class DB_Pool {
    private:
        DB_Config config;

        mutable mutex      mtx;
        condition_variable cv;
        
        struct Entry {
            unique_ptr<connection> conn;
            time_point             last_used;
        };
        
        queue<Entry>   idle;
        atomic<size_t> total{0};
    
        /**
         * @brief Open a new pqxx::connection according to the pool config.
         * @return unique_ptr to the opened connection.
         */
        unique_ptr<connection> openConnection();

        /**
         * @brief Check whether a connection is alive.
         * @param c Connection to check.
         * @return true if the connection appears usable, false otherwise.
         */
        bool isConnectionAilve(connection& c) noexcept;

        /**
         * @brief Prepare connection settings (search_path, timeouts, etc.).
         * @param c Connection to prepare.
         */
        void prepareAll(connection& c);

    public:
        /**
         * @brief Construct a DB_Pool with given configuration.
         * @param cfg Pool configuration.
         */
        explicit DB_Pool(DB_Config cfg);

        /**
         * @brief Destructor; cleans up connections.
         */
        ~DB_Pool();

        DB_Pool           (const DB_Pool&) = delete;
        DB_Pool& operator=(DB_Pool&)       = delete;
        DB_Pool           (DB_Pool&&)      = delete;
        DB_Pool& operator=(DB_Pool&&)      = delete;

        /**
         * @brief Acquire a connection wrapper from the pool.
         * @return DB_Connection that will return the underlying connection
         *         to the pool when destroyed.
         */
        DB_Connection acquire();

        struct Stats {
            size_t total;
            size_t idle;
            size_t inUse;
        };

        /**
         * @brief Get current pool statistics.
         * @return Stats structure with total, idle and in-use counts.
         */
        Stats stats() const noexcept;

        /**
         * @brief Reset the pool, closing idle connections and resetting state.
         */
        void reset();

        /**
         * @brief Return a connection to the pool.
         * @param conn Unique pointer to the connection being released.
         */
        void release(unique_ptr<connection> conn);
};