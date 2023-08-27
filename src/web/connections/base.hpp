#pragma once

#include "common.hpp"
#include "web/connections/base.hpp"

#include <curl/curl.h>
#include <curl/easy.h>

#include <algorithm>

namespace raccoon {
namespace web {

/**
 * A internet connection using libcurl.
 *
 * It is UNDEFINED BEHAVIOR to call any instance methods until
 * conn->ready() returns true.
 */
class Connection {
    CURL* curl_handle_;             // interface to libcurl
    std::string curl_error_buffer_; // error buffer for libcurl

    std::string url_; // url we are requesting from

    bool open_ = false;  // if the connection is open
    bool ready_ = false; // if the connection is ready to open

public:
    /* No copy operators. */
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = default;

    /* Default move operators. */
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;

    /**
     * Destructor that cleans up the curl handle.
     */
    virtual ~Connection() noexcept { curl_easy_cleanup(curl_handle_); }

    /**
     * If our connection is open.
     */
    [[nodiscard]] bool
    open() const noexcept
    {
        return open_;
    }

    /**
     * If our connection has been fully initialized.
     */
    [[nodiscard]] bool
    ready() const noexcept
    {
        return ready_;
    }

    /**
     * The URL this connection is for.
     */
    [[nodiscard]] const auto&
    url() const noexcept
    {
        return url_;
    }

    /**
     * Get a file associated with this request, if there is one.
     *
     * Otherwise, returns nullptr.
     */
    [[nodiscard]] virtual FILE* file() const noexcept = 0;

    friend class RequestManager;

protected:
    /**
     * Create a new connection.
     *
     * Only for subclasses.
     *
     * @param url The url to connect to.
     */
    explicit Connection(std::string url) : Connection(std::move(url), curl_easy_init())
    {}

    /**
     * Create a new connection with specified handle.
     *
     * Only for subclasses.
     *
     * @param url The url to connect to.
     * @param curl_handle The curl handle to use for this request.
     */
    Connection(std::string url, CURL* curl_handle) :
        curl_handle_(curl_handle),
        curl_error_buffer_(CURL_ERROR_SIZE, '\0'),
        url_(std::move(url))
    {
        if (!curl_handle_) {
            throw std::runtime_error(
                "Could not create cURL handle, or null handle provided."
            );
        }

        // Provide error buffer for cURL
        curl_easy_setopt(curl_handle_, CURLOPT_ERRORBUFFER, curl_error_buffer_.data());
        curl_easy_setopt(
            curl_handle_,
            CURLOPT_VERBOSE,
            raccoon::logging::get_libcurl_logger()->should_log<quill::LogLevel::Debug>()
        );
    }

    /**
     * Start this connection.
     */
    virtual void start_() = 0; // NOLINT(*-naming)

    /**
     * Clear the libcurl error buffer.
     */
    void
    clear_error_buffer_() noexcept
    {
        curl_error_buffer_[0] = '\0';
    }

    /**
     * Handle any errors in libcurl.
     */
    void
    process_curl_error_(CURLcode error_code)
    {
        log_e(
            libcurl,
            "{}: {} (Code {})",
            url_,
            curl_error_buffer_.empty() ? curl_easy_strerror(error_code)
                                       : curl_error_buffer_.c_str(),
            static_cast<unsigned>(error_code)
        );
    }

    /**************************************************************************
     *                           SUBCLASS ACCESSORS                           *
     *************************************************************************/
    // NOLINTBEGIN(*-naming)

    bool&
    open() noexcept
    {
        return open_;
    }

    [[nodiscard]] auto*
    curl_handle() noexcept
    {
        return curl_handle_;
    }

    // NOLINTEND(*-naming)
};

} // namespace web
} // namespace raccoon
