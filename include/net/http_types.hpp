#pragma once

#include <string>
#include <unordered_map>

using std::string;
using std::unordered_map;

struct http_request {
    string                        method;
    string                        path;
    string                        version;
    
    unordered_map<string, string> headers;

    string                        body;
    
    void parse(const string& raw);
};

struct http_response {
    string                        version;
    int                           status_code;
    string                        status_txt;

    unordered_map<string, string> headers;

    string                        body;

    string toString() const;
};