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
     * Close the connection.
     */
    virtual void close() = 0;

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
    Connection(const std::string& url, CURL* curl_handle);

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
    void process_curl_error_(CURLcode error_code);

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
