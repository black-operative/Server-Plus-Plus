#include <string>
#include <chrono>
#include <vector>

using std::string;
using std::vector;
using std::chrono::seconds;
using std::chrono::milliseconds;

struct DB_Config {
    string       connString;
    size_t       minConnections = 1;
    size_t       maxConnections = 10;
    milliseconds acquireTimeout {5000};
    seconds      idleTimeout    {300};

    struct PreparedStatement { string name, sql; };
    vector<PreparedStatement> preparedStatements;

};