#include "../../include/net/route.hpp"
#include "../../include/net/router.hpp"

#include <regex>

using std::regex_match;
using std::smatch;

Route::Route(
    const string& method,
    const string& path,
    route_handler& handler
) : 
    method(method), 
    handler(handler) 
{
    path_pattern = regex(
        Router::path_to_regex(
            path, 
            param_names
        )
    );
}

bool Route::match(
    const string& method,
    const string& path,
    route_parameters& params
) const {
    if (this->method != method) return false;
    
    smatch matches;
    if (
        !regex_match(
            path, 
            matches, 
            path_pattern
        )
    ) return false;

    for (size_t i = 0; i < param_names.size(); i++) {
        params[param_names[i]] = matches[i + 1].str();
    }

    return true;
}