#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <vector>

#include <pqxx/pqxx>

using std::atomic;
using std::vector;
using std::string;
using std::unique_ptr;

using pqxx::work;
using pqxx::result;
using pqxx::connection;
using pqxx::nontransaction;
using pqxx::read_transaction;

//  Forward declaration  (breaks the circular dependency with DB_Pool)
class DB_Pool;

//  PgConnection  –  Checked-out connection handle
//
//  Returned exclusively by PgPool::acquire().
//  Automatically returns the underlying pqxx::connection to the pool when
//  it goes out of scope (RAII).
//
//  pqxx::connection is NOT thread-safe — each thread must hold its own
//  PgConnection for the duration of its work.
class DB_Connection {
    private:
        unique_ptr<connection> conn;
        DB_Pool*               pool = nullptr;

    public:
        DB_Connection(unique_ptr<connection> conn, DB_Pool& pool);
        ~DB_Connection();

        DB_Connection           (const DB_Connection& ) = delete;
        DB_Connection& operator=(const DB_Connection& ) = delete;
        DB_Connection           (      DB_Connection&&) noexcept;
        DB_Connection& operator=(      DB_Connection&&) noexcept;

        // ── Read-write transaction ────────────────────────────────────────────────
        //
        //   auto result = conn.work([](work& tx) {
        //       return tx.exec("SELECT now()");
        //   });
        //
        // Commits on success, rolls back automatically on exception.
        template <typename Fn>
        auto work(Fn&& fn) -> decltype(fn(std::declval<work&>())) {
            pqxx::work transaction(*this->conn);
            try {
                auto result = fn(transaction);
                transaction.commit();
                return result;
            } catch (...) {
                throw;
            }
        }

        // ── Read-only transaction ─────────────────────────────────────────────────
        //
        //   auto result = conn.readOnly([](read_transaction& tx) {
        //       return tx.exec("SELECT id, name FROM users");
        //   });
        template <typename Fn>
        auto readOnly(Fn&& fn) -> decltype(fn(std::declval<read_transaction&>())) {
            read_transaction tx(*this->conn);
            return fn(tx);   
        }

        // ── Non-transactional execution (DDL, COPY, SET, etc.) ───────────────────
        result execNonTx(const std::string& sql);

        // ── Prepared statements ──────────────────────────────────────────────────
        //
        // Prepare a named statement on this connection (idempotent per name).
        void prepare(const std::string& name, const std::string& sql);
    
        // Execute a prepared statement inside an auto-committed work transaction.
        // Pass arguments via params, e.g.:
        //
        //   conn.execPrepared("find_user", params{42, "alice"});
        //
        template <typename... Args>
        result execPrepared(const std::string& name, Args&&... args) {
            pqxx::work tx(*this->conn);
            auto result = tx.exec_prepared(
                name, 
                std::forward<Args>(args)...
            );
            tx.commit();
            return result;
        }

        // ── Diagnostics ──────────────────────────────────────────────────────────
        bool isAlive() const noexcept;
    
        /// Direct access to the underlying connection (use sparingly)
        connection& raw() { return *this->conn; }

};