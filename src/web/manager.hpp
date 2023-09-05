#pragma once

#include "common.hpp"
#include "connections/connections.hpp"
#include "logging.hpp"
#include "web/connections/base.hpp"

#include <curl/curl.h>
#include <curl/multi.h>
#include <uv.h>

#include <queue>

namespace raccoon {
namespace web {

class RequestManager;

/**
 * Implementation details.
 */
namespace detail {

/**
 * Friend function to access the handle of the request manager.
 */
CURLM* get_handle(RequestManager* manager);

/**
 * Friend function to access a private method of the request manager.
 */
void process_libcurl_messages(RequestManager* manager, int running_handles);

} // namespace detail

/**
 * Class to manage web requests.
 */
class RequestManager {
    CURLM* curl_handle_; // curl multi handle

    uv_loop_t* loop_;      // uv loop handle
    uv_timer_t timeout_{}; // curl socket timeout handle

    // Deferred list of connections to add to libcurl
    // As we cannot add them in callbacks
    // So we add them on timeout of a libuv timer
    std::queue<std::shared_ptr<Connection>> connections_to_init_{};
    uv_timer_t init_task_timer_{}; // timer to run connection initialization

    // list of all initialized connections
    std::vector<std::shared_ptr<Connection>> connections_;

public:
    /**
     * Create a new request manager.
     */
    explicit RequestManager(uv_loop_t* event_loop = uv_default_loop());

    /**
     * Run the session to completion.
     */
    void
    run() noexcept
    {
        log_i(web, "Starting web session");
        log_bt(web, "Starting session for handle {}", fmt::ptr(curl_handle_));

        uv_run(loop_, UV_RUN_DEFAULT);
    }

    /**
     * Open a WebSocket connection.
     *
     * @param url The url to open a connection to.
     * @param callback A callback to process received data.
     *
     * @returns std::shared_ptr<WebSocketConnection> The web socket connection.
     */
    std::shared_ptr<WebSocketConnection>
    ws(const std::string& url, WebSocketConnection::callback on_data);

    /**
     * Get all initialized connections.
     *
     * @returns std::vector<std::shared_ptr<WebSocketConnection>>
     */
    [[nodiscard]] auto
    connections() const
    {
        // Return a copy so that people cannot remove connections
        return connections_;
    }

    /* Friends */
    friend CURLM* detail::get_handle(RequestManager* manager);

    friend void
    detail::process_libcurl_messages(RequestManager* manager, int running_handles);

private:
    static void run_initializations_(uv_timer_t* handle); // NOLINT(*-naming)

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

    void process_libcurl_messages_();
};

} // namespace web
} // namespace raccoon
