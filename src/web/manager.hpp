#pragma once

#include "common.hpp"
#include "logging.hpp"
#include "web/ws.hpp"
#include "ws.hpp"

#include <curl/curl.h>
#include <curl/multi.h>
#include <uv.h>

namespace raccoon {
namespace web {

class RequestManager;

namespace detail {

CURLM* get_handle(RequestManager* manager);

void log_curl_status(RequestManager* manager, int running_handles);

} // namespace detail

/**
 * Class to manage web requests.
 */
class RequestManager {
    CURLM* curl_handle_;

    uv_loop_t* loop_;
    uv_timer_t timeout_{};

public:
    /**
     * Create a new request manager.
     */
    explicit RequestManager(uv_loop_t* event_loop = uv_default_loop());

    /**
     * Run the session to completion.
     */
    void
    run()
    {
        uv_run(loop_, UV_RUN_DEFAULT);
    }

    /**
     * Open a WebSocket connection.
     *
     * @param url The url to open a connection to.
     * @param callback A callback to process received data.
     */
    std::unique_ptr<WebSocketConnection>
    ws(std::string url, WebSocketConnection::callback on_data);

    friend CURLM* detail::get_handle(RequestManager* manager);
    friend void detail::log_curl_status(RequestManager* manager, int running_handles);

private:
    static int handle_socket_( // NOLINT(*-naming)
        CURL* easy,
        curl_socket_t sock_fd,
        int action,
        void* user_ptr,
        void* socket_ptr
    );

    static int start_timeout_( // NOLINT(*-naming)
        CURLM* multi,
        int32_t timeout_ms,
        void* user_ptr
    );

    void log_status_();
};

} // namespace web
} // namespace raccoon
