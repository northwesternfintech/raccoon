#include "web.hpp"

#include <curl/curl.h>

namespace raccoon {
namespace utils {

std::string
normalize_url(const std::string& url) noexcept
{
    // Set our old url
    CURLU* handle = curl_url();
    curl_url_set(handle, CURLUPART_URL, url.c_str(), 0);

    // Get our new, normalized url
    char* new_url{};
    curl_url_get(handle, CURLUPART_URL, &new_url, 0);

    // Save normalized URL
    std::string res = new_url;

    // Cleanup
    curl_free(new_url);
    curl_url_cleanup(handle);

    // Return new url
    log_t3(web, "Normalized URL `{}` to `{}`", url, res);

    return res;
}

} // namespace utils
} // namespace raccoon
