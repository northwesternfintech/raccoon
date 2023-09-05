#pragma once

#include "common.hpp"
#include "web.hpp"

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

/**
 * Return a hexdump of some data.
 */
std::string hexdump(void* data, size_t size);

} // namespace utils
} // namespace raccoon
