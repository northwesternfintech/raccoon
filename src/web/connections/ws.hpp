#pragma once

#include "base.hpp"
#include "common.hpp"
#include "web/connections/base.hpp"

#include <functional>

namespace raccoon {
namespace web {

enum class WebSocketCloseStatus : uint16_t {
    NORMAL = 1000,                // request was fulfilled
    ENDPOINT_UNAVAILABLE = 1001,  // either client or server will become unavailable
    PROTOCOL_ERROR = 1002,        // someone made a protocol error
    INVALID_MESSAGE_TYPE = 1003,  // invalid message type for device
    EMPTY = 1005,                 // no error specified
    ABNORMAL_CLOSURE = 1006,      // connection closed abnormally
    INVALID_PAYLOAD = 1007,       // data inconsistent with message type
    POLICY_VIOLATION = 1008,      // endpoint received message against policy
    MESSAGE_TOO_BIG = 1009,       // message too big to process
    MANDATORY_EXTENSION = 1010,   // server didn't negotiate extension
    INTERNAL_SERVER_ERROR = 1011, // server encountered an error
    TLS_HANDSHAKE = 1015          // we're doing SSL
};

/**
 * A websocket connection.
 *
 * It is UNDEFINED BEHAVIOR to call any instance methods until
 * conn->ready() returns true.
 */
class WebSocketConnection : public Connection {
public:
    /**
     * A websocket callback function.
     *
     * Parameters are this class, data buffer, and data length.
     */
    using callback =
        std::function<void(WebSocketConnection*, const std::vector<uint8_t>&)>;

private:
    std::vector<uint8_t> write_buf_;
    callback on_data_;

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
    ~WebSocketConnection() override
    {
        if (open())
            close();
    }

    /**
     * Close this websocket connection.
     */
    void
    close() override
    {
        close(WebSocketCloseStatus::NORMAL);
    }

    /**
     * Close this websocket connection with reason.
     */
    void
    close(WebSocketCloseStatus status)
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
     * Dummy, websockets don't download to files.
     */
    [[nodiscard]] FILE*
    file() const noexcept override
    {
        return nullptr;
    }

    friend class RequestManager;

private:
    /**
     * Create a new websocket connection.
     *
     * Should only be called by the RequestManager.
     */
    WebSocketConnection(const std::string& url, callback on_data) :
        Connection(url), on_data_(std::move(on_data))
    {}

    /**
     * Start this websocket connection.
     */
    void start_() override;

    /**
     * Receive data from libcurl, and pass it on to the user callback.
     */
    static size_t write_callback_( // NOLINT(*-identifier-naming)
        uint8_t* buf,
        size_t elem_size,
        size_t length,
        void* user_data
    );
};

/**
 * {fmt} formatting function for WebSocketCloseStatus.
 */
inline auto
format_as(WebSocketCloseStatus status)
{
    return fmt::underlying(status);
}

} // namespace web

} // namespace raccoon
