#include "db/internal/db_conn.hpp"
#include "db/internal/db_pool.hpp"
#include <stdexcept>

DB_Connection::DB_Connection(
    unique_ptr<connection> conn, 
    DB_Pool& pool
) : conn(std::move(conn))
  , pool(&pool)
{}

DB_Connection::~DB_Connection() {
    if (this->conn && this->pool)
        this->pool->release(std::move(conn));
}

DB_Connection::DB_Connection(DB_Connection&& other) noexcept 
    : conn(std::move(other.conn))
    , pool(other.pool)
{
    other.pool = nullptr;
}

DB_Connection& DB_Connection::operator=(DB_Connection&& other) noexcept {
    if (this != &other) {
        if (this->conn && this->pool)
            this->pool->release(std::move(conn));

        this->conn = std::move(other.conn);
        this->pool = other.pool;
        other.pool = nullptr;
    }
    return *this;
}

result DB_Connection::execNonTx(const string& sql) {
    if (!this->conn)
        throw std::runtime_error("[Database] @ execNonTx : No active connection");

    nontransaction n_transaction(*this->conn);
    return n_transaction.exec(sql);
}

void DB_Connection::prepare(
    const string& name, 
    const string& sql
) {
    if (!this->conn) 
        throw std::runtime_error("[Database] @ prepare : No active connection");

    this->conn->prepare(name, sql);
}

bool DB_Connection::isAlive() const noexcept {
    if (!this->conn)
        return false;

    try {
        nontransaction n_transaction(*this->conn);
        n_transaction.exec("SELECT 1");
        return true;
    } catch (...) {
        return false;
    }
}
