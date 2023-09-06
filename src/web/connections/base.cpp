#include "base.hpp"

#include "logging.hpp"

#include <curl/curl.h>
#include <quill/LogLevel.h>

#include <cctype>

namespace raccoon {
namespace web {

Connection::Connection(const std::string& url, CURL* curl_handle) :
    curl_handle_(curl_handle),
    curl_error_buffer_(CURL_ERROR_SIZE, '\0'),
    url_(utils::normalize_url(url))
{
    log_bt(web, "Initialize conn object and curl handle for {}", url_);

    if (!curl_handle_) [[unlikely]] {
        throw std::runtime_error(
            "Could not create cURL handle, or null handle provided."
        );
    }

    // Pass this class with the curl handle
    curl_easy_setopt(curl_handle_, CURLOPT_PRIVATE, this);

    // Add a debug function
    curl_easy_setopt(curl_handle_, CURLOPT_DEBUGFUNCTION, debug_log_callback_);
    curl_easy_setopt(curl_handle_, CURLOPT_DEBUGDATA, this);

    // Enable curl logging
    curl_easy_setopt(
        curl_handle_,
        CURLOPT_VERBOSE,
        raccoon::logging::get_libcurl_logger()->should_log<quill::LogLevel::Debug>()
    );

    // Provide error buffer for cURL
    curl_easy_setopt(curl_handle_, CURLOPT_ERRORBUFFER, curl_error_buffer_.data());
}

int
Connection::debug_log_callback_(
    CURL* handle,       // curl handle emitting the log
    curl_infotype type, // type of log
    char* raw_data,     // log data
    size_t size,        // log data size
    void* userdata      // user data pointer
)
{
    UNUSED(handle);

    // Get connection
    auto* conn = static_cast<Connection*>(userdata);

    switch (type) {
        case CURLINFO_TEXT:
            {
                std::string data(raw_data, size - 1); // drop newline
                log_d(libcurl, "[{}] {}", conn->url_, data);
                return 0;
            }

        case CURLINFO_HEADER_OUT:
            log_t1(libcurl, "{} <== Send header", conn->url_);
            break;

        case CURLINFO_DATA_OUT:
            log_t1(libcurl, "{} <== Send data", conn->url_);
            break;

        case CURLINFO_SSL_DATA_OUT:
            log_t1(libcurl, "{} <== Send SSL data", conn->url_);
            break;

        case CURLINFO_HEADER_IN:
            log_t1(libcurl, "{} ==> Recv header", conn->url_);
            break;

        case CURLINFO_DATA_IN:
            log_t1(libcurl, "{} ==> Recv data", conn->url_);
            break;

        case CURLINFO_SSL_DATA_IN:
            log_t1(libcurl, "{} ==> Recv SSL data", conn->url_);
            break;

        [[unlikely]] default:
            return 0;
    }

    if (logging::get_libcurl_logger()->should_log<quill::LogLevel::TraceL3>())
        log_t3(libcurl, "Hexdump\n{}", utils::hexdump(raw_data, size));

    return 0;
}

void
Connection::process_curl_error_(CURLcode error_code)
{
    log_e(
        libcurl,
        "[{}] {} (Code {})",
        url_,
        curl_error_buffer_.empty() ? curl_easy_strerror(error_code)
                                   : curl_error_buffer_.c_str(),
        static_cast<unsigned>(error_code)
    );
}

} // namespace web
} // namespace raccoon
