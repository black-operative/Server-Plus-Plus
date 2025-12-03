#include <fstream>
#include <iostream>

#include "util/dot_env.hpp"

using std::cerr;
using std::string;
using std::getline;
using std::ifstream;

static inline string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";

    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static inline bool starts_with(const string& s, char c) {
    return !s.empty() && s[0] == c;
}

string getValue(
    const string& key,
    const string& file
) {
    if (key.empty()) {
        cerr << "\n[DOT_ENV] : NULL key argument\n";
        return "";
    }

    std::ifstream reader(file);
    if (!reader.is_open()) {
        cerr << "\n[DOT_ENV] : Cannot open file : " << file << "\n";
        return "";
    }

    string line;
    while (getline(reader, line)) {
        // Remove whitespace
        line = trim(line);

        // Skip empty lines or comments (# or ;)
        if (
            line.empty()                || 
            starts_with(line, '#') || 
            starts_with(line, ';')
        )
            continue;

        // Find delimiter
        size_t pos = line.find('=');
        if (pos == string::npos)
            continue; // invalid line

        string k = trim(line.substr(0, pos));
        string v = trim(line.substr(pos + 1));

        // Remove surrounding quotes if present
        if (
            !v.empty() && 
            (v.front() == '"' && v.back() == '"')
        )
            v = v.substr(1, v.size() - 2);

        if (k == key)
            return v;
    }

    return "";
}