#include <string>

#include "../../include/net/http_types.hpp"

using std::to_string;

void http_request::parse(const string& raw) {
    headers.clear();
    body.clear();
    
    size_t pos = 0;
    
    // Parse request line (Eg : GET /path HTTP/1.1)
    //                             |     |
    //                   first_space     second_space

    size_t line_end = raw.find("\r\n", pos);
    if (line_end == string::npos) return;
    string request_line = raw.substr(pos, line_end - pos);
    
    
    // Extract method, path, version
    size_t first_space  = request_line.find(' ');
    size_t second_space = request_line.find(' ', first_space + 1);
    
    if (
        first_space != string::npos && 
        second_space != string::npos
    ) {
        this->method  = request_line.substr(0, first_space);
        this->path    = request_line.substr(first_space + 1, second_space - first_space - 1);
        this->version = request_line.substr(second_space + 1);
    }
    
    // Move past \r\n
    pos = line_end + 2; 
    
    // Parse headers
    while (pos < raw.length()) {
        line_end = raw.find("\r\n", pos);
        if (line_end == string::npos) break;
        
        // Empty line indicates end of headers
        if (line_end == pos) {
            // Skip \r\n
            pos += 2; 
            break;
        }
        
        string header_line = raw.substr(pos, line_end - pos);
        
        size_t colon = header_line.find(':');

        if (colon != string::npos) {
            string key   = header_line.substr(0, colon);
            string value = header_line.substr(colon + 1);
            
            // Trim leading/trailing whitespace from value
            size_t value_start = value.find_first_not_of(" \t");
            size_t value_end   = value.find_last_not_of(" \t");
            
            if (
                value_start != string::npos && 
                value_end   != string::npos
            ) {
                value = value.substr(
                    value_start, 
                    value_end - value_start + 1
                );
            } else if (value_start != string::npos) {
                value = value.substr(value_start);
            } else {
                value = "";
            }
            
            headers[key] = value;
        }
        
        // Move past \r\n
        pos = line_end + 2; 
    }
    
    // Parse body (everything after headers)
    if (pos < raw.length()) {
        body = raw.substr(pos);
    }
}

string http_response::toString() const {
    string result;
    
    result += version + " " + std::to_string(status_code) + " " + status_txt + "\r\n";
    
    for (const auto& header : this->headers) {
        result += 
            header.first  + 
            ": "          + 
            header.second + 
            "\r\n";
    }
    
    if (
        headers.count("Content-Length") == 0 && 
        !body.empty()
    ) {
        result += 
            "Content-Length: "       + 
            to_string(body.length()) + 
            "\r\n";
    }
    
    result += "\r\n";
    
    result += body;
    
    return result;
}