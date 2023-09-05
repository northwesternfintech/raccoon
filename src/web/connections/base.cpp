#include "base.hpp"

#include "logging.hpp"

#include <curl/curl.h>
#include <quill/LogLevel.h>

#include <cctype>

namespace raccoon {
namespace web {

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

} // namespace web
} // namespace raccoon
