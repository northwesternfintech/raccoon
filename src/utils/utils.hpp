#pragma once

#include "common.hpp"

namespace raccoon {
namespace utils {

/**
 * Read an environment variable, with an optional default.
 */
inline std::string
getenv(const std::string& variable, const std::string& default_val)
{
    const char* value = std::getenv(variable.c_str()); // NOLINT(concurrency-*)
    return value ? std::string(value) : default_val;
}

} // namespace utils
} // namespace raccoon
