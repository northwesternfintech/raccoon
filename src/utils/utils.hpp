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
std::string hexdump(const void* data, size_t size);

/**
 * Return a hexdump of a vector of data.
 */
template <class T>
inline std::string
hexdump(const std::vector<T>& data)
{
    return hexdump(data.data(), data.size() * sizeof(T));
}

/**
 * Return a hexdump of a string.
 */
inline std::string
hexdump(const std::string& data)
{
    return hexdump(data.c_str(), data.size());
}

} // namespace utils
} // namespace raccoon
