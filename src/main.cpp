#include <string>

#include "net/server.hpp"
#include "util/dot_env.hpp"

using std::stoi;

int main(int argc, char** argv) {
    Server server(stoi(getValue("PORT")));

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

    server.start();

    return 0;
}