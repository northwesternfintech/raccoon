#include <libwebsockets.h>

#include <cstdint>

#include <array>
#include <functional>
#include <string>

namespace raccoon {

namespace web {

/**
 * A websocket connection.
 */
class WebSocketConnection {
public:
    /**
     * A websocket callback function.
     *
     * Parameters are this class, data buffer, and data length.
     *
     * Return value should be 0 unless you're doing something weird
     * (closing the connection, etc.).
     */
    using callback = int (*)(WebSocketConnection*, void*, size_t);

private:
    struct lws* wsi_; // libwebsocket information

    lws_sorted_usec_list_t retry_list_; // collection of retries
    uint16_t retry_count_;              // connection retry count

    bool closed_ : 1 = true;     // if we are purposefully closed
    bool connected_ : 1 = false; // if we are connected to the server

    callback on_data_; // Data callback

    std::string address_;  // server address
    uint16_t port_;        // server port
    std::string path_;     // websocket path
    std::string protocol_; // websocket protocol

public:
    // NOLINTNEXTLINE(readability-identifier-naming, *-avoid-c-arrays)
    static const struct lws_protocols CONNECTION_PROTOCOLS[];

    /**
     * Create a new websocket connection.
     */
    WebSocketConnection(
        const std::string& address,
        const std::string& path,
        const std::string& protocol,
        const callback& on_data
    ) : // NOLINTNEXTLINE(*-magic-numbers): port 443 is SSL
        WebSocketConnection(address, 443, path, protocol, on_data)
    {}

    /**
     * Create a new websocket connection.
     */
    WebSocketConnection(
        std::string address,
        uint16_t port,
        std::string path,
        std::string protocol,
        callback on_data
    );

    /* No copy operators */
    WebSocketConnection(const WebSocketConnection&) = delete;
    WebSocketConnection& operator=(const WebSocketConnection&) = delete;

    /* Default move operators */
    WebSocketConnection(WebSocketConnection&&) = default;
    WebSocketConnection& operator=(WebSocketConnection&&) = default;

    /**
     * Close this websocket connection.
     */
    ~WebSocketConnection() noexcept { close(LWS_CLOSE_STATUS_NORMAL); }

    /**
     * Open this websocket connection if closed.
     */
    void open() noexcept;

    /**
     * Close this websocket connection with reason.
     */
    void
    close(lws_close_status status) noexcept
    {
        close(status, nullptr, 0);
    }

    /**
     * Close this websocket connection with reason and data.
     */
    void
    close(lws_close_status status, uint8_t* data, size_t len) noexcept
    {
        if (closed_)
            return;

        lws_close_reason(wsi_, status, data, len);
        closed_ = true;
    }

    /**
     * If we are purposefully closed.
     */
    [[nodiscard]] bool
    closed() const noexcept
    {
        return closed_;
    }

    /**
     * If we are connected to the server.
     */
    [[nodiscard]] bool
    connected() const noexcept
    {
        return connected_;
    }

private:
    // NOLINTNEXTLINE(readability-identifier-naming)
    static void connect_(lws_sorted_usec_list_t* retry_list);

    // NOLINTNEXTLINE(readability-identifier-naming)
    static int lws_callback_(
        struct lws* wsi,
        enum lws_callback_reasons reason,
        void* user,
        void* data,
        size_t len
    );
};

/**
 * Initialize libwebsocket.
 */
int initialize(int argc, const char* argv[]); // NOLINT(*-c-arrays)

/**
 * Run the libwebsocket main loop.
 */
int run();

} // namespace web

} // namespace raccoon
