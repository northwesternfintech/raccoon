#include "manager.hpp"

#include "common.hpp"

#include <curl/curl.h>
#include <uv.h>

namespace raccoon {
namespace web {

RequestManager::RequestManager(uv_loop_t* event_loop) :
    curl_handle_(curl_multi_init()), loop_(event_loop)
{
    // Set up callbacks
    curl_multi_setopt(curl_handle_, CURLMOPT_SOCKETFUNCTION, handle_socket_);
    curl_multi_setopt(curl_handle_, CURLMOPT_TIMERFUNCTION, start_timeout_);

    // Pass this class instance as user data to callbacks
    curl_multi_setopt(curl_handle_, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(curl_handle_, CURLMOPT_TIMERDATA, this);

    // Set up our curl timer
    uv_timer_init(loop_, &timeout_);
    timeout_.data = this;

    // Setup up our initialization timer and start it
    uv_timer_init(loop_, &init_task_timer_);
    init_task_timer_.data = this;
}

void
RequestManager::process_libcurl_messages_()
{
    CURLMsg* message{};
    int remaining_messages{};

    while ((message = curl_multi_info_read(curl_handle_, &remaining_messages))) {
        switch (message->msg) {
            case CURLMSG_DONE:
                {
                    /* Do not use message data after calling curl_multi_remove_handle()
                     * and curl_easy_cleanup(). As per curl_multi_info_read() docs:
                     *
                     * "WARNING: The data the returned pointer points to will not
                     *  survive calling curl_multi_cleanup, curl_multi_remove_handle or
                     *  curl_easy_cleanup."
                     */
                    CURL* easy_handle = message->easy_handle;

                    // Get our connection
                    char* data{};
                    curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &data);

                    auto* conn = reinterpret_cast<Connection*>(data);
                    assert(conn->curl_handle_ == easy_handle);

                    // Log that the request finished
                    char* url{};
                    curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &url);

                    log_i(libcurl, "Connection to {} finished", url);

                    // Log any errors
                    auto err = message->data.result; // NOLINT(*-union-access)
                    if (err) {
                        log_w(web, "Found errors in connection to {}", url);
                        conn->process_curl_error_(err);
                    }

                    // Remove the handle and clean it up
                    curl_multi_remove_handle(curl_handle_, easy_handle);
                    curl_easy_cleanup(easy_handle);

                    conn->curl_handle_ = nullptr; // freed the handle

                    // Close any files from the request
                    if (conn->file()) {
                        auto ok = fclose(conn->file());

                        if (!ok)
                            log_e(libcurl, "Error closing file: {}", strerror(errno));
                    }
                    break;
                }

            default:
                log_i(libcurl, "CURLMsg default\n");
                break;
        }
    }
}

/******************************************************************************
 *                              REQUEST METHODS                               *
 *****************************************************************************/

void
RequestManager::run_initializations_(uv_timer_t* handle)
{
    // Get manager
    auto* manager = static_cast<RequestManager*>(handle->data);

    // Run through initialization queue
    auto& init_queue = manager->connections_to_init_;

    while (!init_queue.empty()) {
        // Get our connection
        auto conn = std::move(init_queue.front());
        init_queue.pop();

        log_i(libcurl, "Opening connection to {}", conn->url());

        // Add the connection to the curl multi handle
        auto err = curl_multi_add_handle(manager->curl_handle_, conn->curl_handle_);
        if (err) {
            log_e(
                libcurl,
                "Connection to {} failed: {} (Code {})",
                conn->url(),
                curl_multi_strerror(err),
                static_cast<int64_t>(err)
            );

            // Fail
            // TODO(nino): maybe implement some sort of error recovery?
            abort();
        }

        // Start the connection
        conn->ready_ = true;
        conn->start_();

        // Save it to our connection list
        manager->connections_.push_back(std::move(conn));
    }
}

std::shared_ptr<WebSocketConnection>
RequestManager::ws(std::string url, WebSocketConnection::callback on_data)
{
    // Create the connection
    log_i(web, "Creating WebSocket connection for {}", url);

    auto conn = std::shared_ptr<WebSocketConnection>( // ctor is private to shared_ptr
        new WebSocketConnection(std::move(url), std::move(on_data))
    );

    // Add the connection to our initialization queue
    connections_to_init_.push(conn);

    // Request that the initialization function runs next iteration
    uv_timer_start(&init_task_timer_, run_initializations_, 0, 0);

    // Return the connection to the user
    return conn;
}

/******************************************************************************
 *                               CURL CALLBACKS                               *
 *****************************************************************************/

namespace detail {

CURLM*
get_handle(RequestManager* manager)
{
    return manager->curl_handle_;
}

void
process_libcurl_messages(RequestManager* manager, int running_handles)
{
    log_d(libcurl, "{} running handles", running_handles);
    manager->process_libcurl_messages_();
}

/**
 * Struct linking a curl socket to a uv socket.
 */
struct curl_context_t {
    uv_poll_t poll_handle;
    curl_socket_t sock_fd;

    RequestManager* manager;
};

/**
 * Create a new curl context.
 */
static curl_context_t*
create_curl_context(curl_socket_t sock_fd, uv_loop_t* loop, RequestManager* manager)
{
    // Create our context
    auto* ctx = new curl_context_t;

    // Set the file descriptor
    ctx->sock_fd = sock_fd;

    // Allocate the uvloop handle
    uv_poll_init_socket(loop, &ctx->poll_handle, sock_fd);
    ctx->poll_handle.data = ctx;

    // Attach the request manager
    ctx->manager = manager;

    return ctx;
}

/**
 * Destroy a curl context.
 */
static void
destroy_curl_context(curl_context_t* ctx)
{
    uv_close(
        reinterpret_cast<uv_handle_t*>(&ctx->poll_handle),
        [](uv_handle_t* handle) {
            auto* ctx = static_cast<curl_context_t*>(handle->data);
            delete ctx;
        }
    );
}

static void
on_poll(uv_poll_t* req, int status, int uv_events)
{
    UNUSED(status);

    // Set flags for curl
    uint32_t flags = 0;
    auto events = static_cast<uint32_t>(uv_events);

    if (events & UV_READABLE)
        flags |= CURL_CSELECT_IN;

    if (events & UV_WRITABLE)
        flags |= CURL_CSELECT_OUT;

    // Get our context
    auto* curl_ctx = static_cast<curl_context_t*>(req->data);

    // Report to curl
    int running_handles{};
    curl_multi_socket_action(
        get_handle(curl_ctx->manager),
        curl_ctx->sock_fd,
        static_cast<int>(flags),
        &running_handles
    );

    // Log status
    process_libcurl_messages(curl_ctx->manager, running_handles);
}

} // namespace detail

int
RequestManager::handle_socket_(
    CURL* easy,            // easy handle
    curl_socket_t sock_fd, // socket
    int action,            // what curl wants us to do with the socket
    void* user_ptr,        // private callback pointer
    void* socket_ptr       // private socket pointer
)
{
    UNUSED(easy);

    // Get our request manager
    auto* manager = static_cast<RequestManager*>(user_ptr);

    // Get our socket context
    auto* curl_ctx = static_cast<detail::curl_context_t*>(socket_ptr);

    switch (action) {
        case CURL_POLL_IN:
        case CURL_POLL_OUT:
        case CURL_POLL_INOUT:
            {
                // Allocate a context if necessary
                if (!curl_ctx) {
                    curl_ctx = detail::create_curl_context( //
                        sock_fd,
                        manager->loop_,
                        manager
                    );
                }

                // Assign the context to this socket
                curl_multi_assign(manager->curl_handle_, sock_fd, curl_ctx);

                // Get our libuv events
                uint32_t events = 0;

                if (action != CURL_POLL_IN)
                    events |= UV_WRITABLE;

                if (action != CURL_POLL_OUT)
                    events |= UV_READABLE;

                // Start polling this socket in libuv
                uv_poll_start(
                    &curl_ctx->poll_handle, static_cast<int>(events), detail::on_poll
                );

                break;
            }
        case CURL_POLL_REMOVE:
            {
                if (curl_ctx) {
                    // Stop polling
                    uv_poll_stop(&curl_ctx->poll_handle);

                    // Free this socket context
                    destroy_curl_context(curl_ctx);

                    // Clear the data in libcurl
                    curl_multi_assign(manager->curl_handle_, sock_fd, nullptr);
                }
                break;
            }
        default:
            // Should never be here
            abort();
    }

    return 0;
}

int
RequestManager::start_timeout_( // NOLINT(*-naming)
    CURLM* multi,               // curl multi handle
    int32_t timeout_ms,         // timeout curl wants to set
    void* user_ptr              // pointer to user data
)
{
    UNUSED(multi);

    // Get our request manager
    auto* manager = static_cast<RequestManager*>(user_ptr);

    // Process timer
    if (timeout_ms < 0) { // curl wants to stop the timer
        uv_timer_stop(&manager->timeout_);
    }
    else { // curl wants to create a new timer
        // Check for what curl wants to do
        if (timeout_ms == 0)
            timeout_ms = 1; // call socket_action, bit in a little bit

        // Create our timeout callback
        auto on_timeout = [](uv_timer_t* timer) {
            auto* manager = static_cast<RequestManager*>(timer->data);

            // Process the request
            int running_handles{};
            curl_multi_socket_action(
                manager->curl_handle_, CURL_SOCKET_TIMEOUT, 0, &running_handles
            );

            // Log info
            detail::process_libcurl_messages(manager, running_handles);
        };

        // Start our timer
        uv_timer_start(
            &manager->timeout_, on_timeout, static_cast<uint64_t>(timeout_ms), 0
        );
    }

    return 0;
}

} // namespace web
} // namespace raccoon