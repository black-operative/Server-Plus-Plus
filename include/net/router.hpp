#pragma once

#include <regex>
#include <functional>
#include <unordered_map>

#include "http_types.hpp"
#include "route.hpp"

using std::regex;
using std::vector;
using std::function;
using std::unordered_map;

using middleware = function<
    bool(
        const http_request& req,
        http_response& res
    )
>;

class Router {
    private:
        vector<Route>      routes;
        vector<middleware> middlewares;
        route_handler      not_found_handler;
        route_handler      error_handler;

    public:
        Router();

        void GET   (const string& path  , route_handler handler);
        void POST  (const string& path  , route_handler handler);
        void PUT   (const string& path  , route_handler handler);
        void DELETE(const string& path  , route_handler handler);
        void PATCH (const string& path  , route_handler handler);
        
        void route(
            const string& method, 
            const string& path, 
            route_handler handler
        );

        void use(middleware m);

        void set_not_found(route_handler handler);
        void set_error    (route_handler handler);
        
        void handle(const http_request& req, http_response& res);

        static string path_to_regex(
            const string&   path,
            vector<string>& param_names
        );
};