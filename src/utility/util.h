#ifndef _UTIL_H
#define _UTIL_H

#include <vector>
#include <string>

namespace Util {
    std::vector<std::string> split(
        const std::string & str,
        const std::string & sep = ":");

    std::string trim(const std::string & str);

    std::string join(
        const std::vector<std::string> & pieces,
        const std::string & sep = ":");
};

#endif
