#include "../../include/net/router.hpp"

Router::Router() {
    this->not_found_handler = [] (
        const http_request&     req, 
              http_response&    res, 
        const route_parameters& params
    ) {
        res.status_code = 404;
        res.status_txt  = "Not Found";
        res.body        = "404 - Route not found";
    };

    this->error_handler = [] (
        const http_request&     req, 
              http_response&    res, 
        const route_parameters& params
    ) {
        res.status_code = 500;
        res.status_txt  = "Internal Server Error";
        res.body        = "500 - Internal server error";
    };
}

string Router::path_to_regex(
    const string&   path,
    vector<string>& param_names
) {
    string pattern = "^";
    size_t i       = 0;

    while (i < path.length()) {
        if (path[i] == ':') {
            size_t start = ++i;
            while (i < path.length() && path[i] != '/') i++;

            string param_name = path.substr(start, i - start);
            param_names.push_back(param_name);
            
            pattern += "([^/]+)";
        } else if (path[i] == '*') {
            pattern += ".*";
            i++;
        } else {
            if (string(".^$|()[]{}+?\\").find(path[i]) != string::npos) 
                pattern += "\\";

            pattern += path[i];
            i++;
        }
    }

    pattern += "$";
    return pattern;
}


void Router::GET(
    const string& path, 
    route_handler handler
) {
    route(
        "GET", 
        path, 
        handler
    );
}

void Router::POST(
    const string& path, 
    route_handler handler
) {
    route(
        "POST", 
        path, 
        handler
    );
}

void Router::PUT(
    const string& path, 
    route_handler handler
) {
    route(
        "PUT", 
        path, 
        handler
    );
}

void Router::DELETE(
    const string& path, 
    route_handler handler
) {
    route(
        "DELETE", 
        path, 
        handler
    );
}

void Router::PATCH(
    const string& path, 
    route_handler handler
) {
    route(
        "PATCH", 
        path, 
        handler
    );
}

void Router::route(
    const string& method, 
    const string& path, 
    route_handler handler
) {
    routes.emplace_back(method, path, handler);
}

void Router::use(middleware m) {
    middlewares.push_back(m);
}

void Router::set_not_found(route_handler handler) {
    not_found_handler = handler;
}

void Router::set_error(route_handler handler) {
    error_handler = handler;
}

void Router::handle(const http_request& req, http_response& res) {
    try {
        // Initialize response defaults
        res.version = "HTTP/1.1";
        res.headers["Content-Type"] = "text/plain";
        
        // Run middlewares
        for (auto& mw : this->middlewares) {
            if (!mw(req, res)) {
                return; // Middleware stopped the chain
            }
        }
        
        // Find matching route
        route_parameters params;
        for (const auto& route : this->routes) {
            if (
                route.match(
                    req.method, 
                    req.path, 
                    params
                )
            ) {
                route.handler(req, res, params);
                return;
            }
        }
        
        // No route matched
        not_found_handler(req, res, params);
        
    } catch (...) {
        route_parameters empty_params;
        error_handler(req, res, empty_params);
    }
}