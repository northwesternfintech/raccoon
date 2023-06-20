#include <libwebsockets.h>

#include <array>
#include <string>

namespace raccoon {

namespace web {

/**
 * A websocket connection.
 */
class WebSocketConnection {
    struct lws* wsi_; // libwebsocket information

    lws_sorted_usec_list_t retry_list_; // collection of retries
    uint16_t retry_count_;              // connection retry count

    void (*on_data_)(void* data, size_t len); // Data callback

    std::string address_;
    std::string path_;
    uint16_t port_;

public:
    // NOLINTNEXTLINE(readability-identifier-naming, *-avoid-c-arrays)
    static const struct lws_protocols CONNECTION_PROTOCOLS[];

    /**
     * Create a new websocket connection.
     */
    WebSocketConnection(
        const std::string& address,
        const std::string& path,
        void (*on_data)(void* data, size_t len)
    ) :
        WebSocketConnection(address, path, 443, on_data)
    {}

    WebSocketConnection(
        std::string address,
        std::string path,
        uint16_t port,
        void (*on_data)(void* data, size_t len)
    );

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
