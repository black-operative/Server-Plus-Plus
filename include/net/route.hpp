#pragma once

#include <regex>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include "http_types.hpp"

using std::regex;
using std::string;
using std::vector;
using std::function;
using std::unordered_map;

using route_parameters = unordered_map<string, string>;

using route_handler = function<
    void(
        const http_request& req, 
        http_response& res, 
        const route_parameters& params
    )
>;

struct Route {
    string         method;
    regex          path_pattern;
    vector<string> param_names;
    route_handler  handler;

    Route(
        const string& method,
        const string& path,
        route_handler& handler
    );
    
    bool match(
        const string& method,
        const string& path,
        route_parameters& params
    ) const;
};