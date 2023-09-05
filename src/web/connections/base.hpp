#pragma once

#include "common.hpp"
#include "utils/utils.hpp"
#include "web/connections/base.hpp"

#include <curl/curl.h>

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
    explicit Connection(const std::string& url) : Connection(url, curl_easy_init()) {}

    /**
     * Create a new connection with specified handle.
     *
     * Only for subclasses.
     *
     * @param url The url to connect to.
     * @param curl_handle The curl handle to use for this request.
     */
    Connection(const std::string& url, CURL* curl_handle) :
        curl_handle_(curl_handle),
        curl_error_buffer_(CURL_ERROR_SIZE, '\0'),
        url_(raccoon::utils::normalize_url(url))
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
        log_bt(web, "Clear error buffer for {}", url_);
        curl_error_buffer_[0] = '\0';
    }

    /**
     * Process libcurl debug logs.
     */
    static int debug_log_callback_( // NOLINT(*-naming)
        CURL* handle,               // curl handle emitting the log
        curl_infotype type,         // type of log
        char* raw_data,             // log data
        size_t size,                // log data size
        void* userdata              // user data pointer
    );

    /**
     * Handle any errors in libcurl.
     */
    void
    process_curl_error_(CURLcode error_code)
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
