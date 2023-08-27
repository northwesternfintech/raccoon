#pragma once

#include "common.hpp"

#include <curl/curl.h>
#include <fmt/format.h>

#include <cstdint>

#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace raccoon {
namespace web {

enum class WebSocketCloseStatus : uint16_t {
    normal = 1000,                // request was fulfilled
    endpoint_unavailable = 1001,  // either client or server will become unavailable
    protocol_error = 1002,        // someone made a protocol error
    invalid_message_type = 1003,  // invalid message type for device
    empty = 1005,                 // no error specified
    abnormal_closure = 1006,      // connection closed abnormally
    invalid_payload = 1007,       // data inconsistent with message type
    policy_violation = 1008,      // endpoint received message against policy
    message_too_big = 1009,       // message too big to process
    mandatory_extension = 1010,   // server didn't negotiate extension
    internal_server_error = 1011, // server encountered an error
    tls_handshake = 1015          // we're doing SSL
};

/**
 * A websocket connection.
 *
 * It is UNDEFINED BEHAVIOR to call any instance methods until
 * conn->ready() returns true.
 */
class WebSocketConnection {
public:
    /**
     * A websocket callback function.
     *
     * Parameters are this class, data buffer, and data length.
     */
    using callback =
        std::function<void(WebSocketConnection*, const std::vector<uint8_t>&)>;

private:
    CURL* curl_handle_;
    std::string curl_error_buffer_;

    std::string url_;

    std::vector<uint8_t> write_buf_;
    callback on_data_;

    bool open_ : 1 = false;
    bool ready_ : 1 = false;

public:
    /* No copy operators */
    WebSocketConnection(const WebSocketConnection&) = delete;
    WebSocketConnection& operator=(const WebSocketConnection&) = delete;

    /* Default move operators */
    WebSocketConnection(WebSocketConnection&&) = default;
    WebSocketConnection& operator=(WebSocketConnection&&) = default;

    /**
     * Close this websocket connection.
     */
    ~WebSocketConnection() noexcept
    {
        close();
        curl_easy_cleanup(curl_handle_);
    }

    /**
     * Close this websocket connection with reason.
     */
    void
    close(WebSocketCloseStatus status = WebSocketCloseStatus::normal)
    {
        close(status, {});
    }

    /**
     * Close this websocket connection with reason and data.
     *
     * Returns amount of data written.
     */
    size_t close(WebSocketCloseStatus status, std::vector<uint8_t> data);

    /**
     * Send data to the websocket.
     *
     * Returns the number of bytes sent.
     */
    size_t send(std::vector<uint8_t> data, unsigned flags = CURLWS_TEXT);

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

    friend class RequestManager;

private:
    /**
     * Create a new websocket connection.
     *
     * Should only be called by the RequestManager.
     */
    WebSocketConnection(std::string url, callback on_data) :
        curl_handle_(curl_easy_init()),
        curl_error_buffer_(CURL_ERROR_SIZE, '\0'),
        url_(std::move(url)),
        on_data_(std::move(on_data))
    {
        if (!curl_handle_)
            throw std::runtime_error("Could not create cURL handle");

        // Provide error buffer for cURL
        curl_easy_setopt(curl_handle_, CURLOPT_ERRORBUFFER, curl_error_buffer_.data());
        curl_easy_setopt(curl_handle_, CURLOPT_VERBOSE, 1);
    }

    /**
     * Start this websocket connection.
     */
    void start_();

    /**
     * Dummy, websockets don't download to files.
     */
    FILE*
    file_()
    {
        return nullptr;
    }

    /**
     * Receive data from libcurl, and pass it on to the user callback.
     */
    static size_t write_callback_( // NOLINT(*-identifier-naming)
        uint8_t* buf,
        size_t elem_size,
        size_t length,
        void* user_data
    );

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
};

} // namespace web

} // namespace raccoon
