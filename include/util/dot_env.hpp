#pragma once

#include <string>
using std::string;

string getValue(
    const string& key, 
    const string& file = ".env"
);