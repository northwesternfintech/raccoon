#include "web.hpp"

#include "logging.hpp"

#include <corecrt.h>
#include <libwebsockets.h>

#include <csignal>

#include <array>
#include <utility>

namespace raccoon {
namespace web {

/** If we have been interrupted in some way */
static sig_atomic_t interrupted = false; // NOLINT

/** The LWS context */
static struct lws_context* context = nullptr; // NOLINT

/** Connection retry backoff delay. */
static const std::array<uint32_t, 5> BACKOFF_MS{1000, 2000, 3000, 4000, 5000};

/** Connection retry backoff configuration. */
static const lws_retry_bo_t RETRY_POLICY = {
    .retry_ms_table = BACKOFF_MS.data(),       // Base delay table
    .retry_ms_table_count = BACKOFF_MS.size(), // Delay table size
    .conceal_count = BACKOFF_MS.size(),        // Number of delays to conceal

    .secs_since_valid_ping = 3,    // Force PING after 3 seconds idle
    .secs_since_valid_hangup = 10, // Hangup after 10 seconds idle

    .jitter_percent = 20, // percentage of random jitter
};

// NOLINTNEXTLINE(*-avoid-c-arrays)
const struct lws_protocols WebSocketConnection::CONNECTION_PROTOCOLS[] = {
    {"lws-minimal-client", lws_callback_, 0, 0, 0, nullptr, 0},
    LWS_PROTOCOL_LIST_TERM
};

WebSocketConnection::WebSocketConnection(
    std::string address,
    uint16_t port,
    std::string path,
    std::string protocol,
    callback on_data
) :
    wsi_(nullptr),
    retry_list_{},
    retry_count_(0),
    on_data_(on_data),
    address_(std::move(address)),
    port_(port),
    path_(std::move(path)),
    protocol_(std::move(protocol))
{
    // Schedule the first client connection attempt to happen immediately
    open();
}

void
WebSocketConnection::open() noexcept
{
    if (!closed_)
        return;

    assert(context != nullptr);

    // Clear our retry list and reschedule
    lws_dll2_clear(&retry_list_.list);
    lws_sul_schedule(context, 0, &retry_list_, connect_, 1);

    closed_ = false;
}

void
WebSocketConnection::connect_(lws_sorted_usec_list_t* retry_list)
{
    // Get our connection
    WebSocketConnection* conn =
        lws_container_of(retry_list, WebSocketConnection, retry_list_);

    // Prepare to connect
    struct lws_client_connect_info info {};

    memset(&info, 0, sizeof(info));

    info.context = context;

    info.address = conn->address_.c_str();
    info.host = info.address;
    info.origin = info.address;

    info.port = conn->port_;
    info.path = conn->path_.c_str();

    info.ssl_connection = LCCSCF_USE_SSL | LCCSCF_PRIORITIZE_READS;
    info.retry_and_idle_policy = &RETRY_POLICY;

    info.protocol = conn->protocol_.c_str();
    info.local_protocol_name = "lws-minimal-client";

    info.pwsi = &conn->wsi_;
    info.userdata = conn;

    // Now try to connect
    if (lws_client_connect_via_info(&info)) // success
        return;

    /*
     * Failed... schedule a retry... we can't use the _retry_wsi()
     * convenience wrapper api here because no valid wsi at this
     * point.
     */
    if (conn->closed_) // do nothing if connection is closed already
        return;

    auto fail = lws_retry_sul_schedule(
        context, 0, retry_list, &RETRY_POLICY, connect_, &conn->retry_count_
    );

    if (fail) {
        log_e(web, "LWS connection attempts exhausted for {}", conn->address_);
        interrupted = true;
        conn->closed_ = true;
    }
}

int
WebSocketConnection::lws_callback_(
    struct lws* wsi,
    enum lws_callback_reasons reason,
    void* user,
    void* data,
    size_t len
)
{
    // Get our connection
    auto* conn = static_cast<WebSocketConnection*>(user);

    // See why we were called
    switch (reason) {
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            log_e(
                web,
                "Client Connection Error: {} for {}",
                data ? static_cast<char*>(data) : "(null)",
                conn->address_
            );
            conn->connected_ = false;
            goto do_retry; // NOLINT

        case LWS_CALLBACK_CLIENT_RECEIVE:
            log_d(
                web,
                "{} byte{} of data received from {}",
                len,
                len == 1 ? "" : "s",
                conn->address_
            );
            log_t2(web, "Data: {:#x}", reinterpret_cast<uintptr_t>(data));

            return conn->on_data_(conn, data, len);

        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            log_i(web, "Connection established to {}", conn->address_);
            conn->connected_ = true;
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            log_i(web, "Connection to {} closed", conn->address_);
            conn->connected_ = false;
            goto do_retry; // NOLINT

        default:
            break;
    }

    return lws_callback_http_dummy(wsi, reason, user, data, len);

do_retry:

    if (conn->closed_) // do nothing if connection is closed already
        return 0;

    /*
     * retry the connection to keep it nailed up
     *
     * For this example, we try to conceal any problem for one set of
     * backoff retries and then exit the app.
     *
     * If you set retry.conceal_count to be LWS_RETRY_CONCEAL_ALWAYS,
     * it will never give up and keep retrying at the last backoff
     * delay plus the random jitter amount.
     */

    auto fail = lws_retry_sul_schedule_retry_wsi(
        wsi, &conn->retry_list_, connect_, &conn->retry_count_
    );

    if (fail) {
        log_e(web, "LWS connection attempts exhausted for {}", conn->address_);
        interrupted = true;
        conn->closed_ = true;
    }

    return 0;
}

/*****************************************************************************/

// NOLINTNEXTLINE(*-non-const-global-variables)
static void (*prev_sigint_handler)(int) = nullptr;

static void
sigint_handler(int sig)
{
    interrupted = true;

    // Call previous signal handler if it exists
    // i.e., not SIG_DFL or SIG_IGN
    if (reinterpret_cast<uintptr_t>(prev_sigint_handler) > 1)
        prev_sigint_handler(sig);
}

static void
emit_lwsl_logs_to_quill(int level, const char* line)
{
    // Drop newline
    // This const cast is incredibly cursed
    // But it saves us a memcpy
    // And the memory we're editing is a buffer on the stack so its ok
    size_t end = strlen(line);
    const_cast<char*>(line)[end - 1] = '\0'; // NOLINT(*-const-cast,*pointer-arithmetic)

    // Drop timestamp and log level
    // Log data starts at line[30]
    line += 30; // NOLINT(*-pointer-arithmetic, *-magic-numbers)

    switch (level) {
        case LLL_ERR:
            log_e(lws, "{}", line);
            break;

        case LLL_WARN:
            log_w(lws, "{}", line);
            break;

        case LLL_NOTICE:
        case LLL_USER:
            log_i(lws, "{}", line);
            break;

        case LLL_INFO:
        case LLL_THREAD:
            log_d(lws, "{}", line);
            break;

        case LLL_DEBUG:
        case LLL_LATENCY:
            log_t1(lws, "{}", line);
            break;

        case LLL_EXT:
            log_t2(lws, "{}", line);
            break;

        case LLL_HEADER:
        case LLL_PARSER:
            log_t3(lws, "{}", line);
            break;

        default:
            log_e(lws, "Unknown log: {}", line);
            break;
    }
}

int
initialize(int argc, const char* argv[]) // NOLINT(*-c-arrays)
{
    // Attach signal handler
    prev_sigint_handler = signal(SIGINT, sigint_handler);
    if (prev_sigint_handler == SIG_ERR) {
        log_e(main, "Could not attach signal handler for SIGINT");
        return 1;
    }

    // Create context info
    struct lws_context_creation_info info {};

    memset(&info, 0, sizeof(info));

    // Process command line
    lws_cmdline_option_handle_builtin(argc, argv, &info);

    // Direct logs to quill
    lws_set_log_level(
        static_cast<int>(
            // NOLINTNEXTLINE(*-magic-numbers)
            0xfff // Enable all log levels
        ),
        emit_lwsl_logs_to_quill
    );

    // Set default options
    info.fd_limit_per_thread = FD_LIMIT_PER_THREAD;

    info.port = CONTEXT_PORT_NO_LISTEN;                         // not running a server
    info.protocols = WebSocketConnection::CONNECTION_PROTOCOLS; // handlers

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT // initialize ssl
                   | LWS_SERVER_OPTION_LIBUV;           // use uvloop

    // Create LWS context
    context = lws_create_context(&info);
    if (!context) {
        log_e(main, "LWS initialization failed");
        return 1;
    }

    return 0;
}

int
run()
{
    int run = 0;

    while (run >= 0 && !interrupted)
        run = lws_service(context, 0);

    // Cleanup
    lws_context_destroy(context);
    log_i(main, "Completed");

    return 0;
}

} // namespace web
} // namespace raccoon
