#pragma once

#include "common.hpp"

#include <curl/curl.h>

namespace raccoon {
namespace utils {

/**
 * Normalize a URL using libcurl.
 *
 * @param url The old url.
 *
 * @returns std::string The new URL.
 */
std::string normalize_url(const std::string& url) noexcept;

} // namespace utils
} // namespace raccoon
