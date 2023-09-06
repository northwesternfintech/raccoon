#include "ws.hpp"

#include "common.hpp"
#include "utils/utils.hpp"

#include <curl/curl.h>

#include <chrono>

#include <ratio>

namespace raccoon {
namespace web {

size_t
WebSocketConnection::write_callback_(
    uint8_t* buf, size_t elem_size, size_t length, void* user_data
)
{
    // Compute size
    size_t size = elem_size * length;

    // Get our connection
    auto* conn = static_cast<WebSocketConnection*>(user_data);
    assert(conn->ready());

    // Get our frame
    const auto* frame = curl_ws_meta(conn->curl_handle());

    if (!frame) [[unlikely]] {
        log_c(
            web,
            "In WebSocket write callback for {} but got NULL frame, aborting!",
            conn->url()
        );
        abort();
    }

    // Populate backtrace
    log_bt(
        web,
        "WS write callback for url {} with buf of len {} ({}x{}), {} bytes left",
        conn->url(),
        size,
        elem_size,
        length,
        frame->bytesleft
    );

    // Log information
    log_t1(
        web,
        "WebSocket data callback ran for {} ({} bytes, {} left)",
        conn->url(),
        size,
        frame->bytesleft
    );

    if (logging::get_web_logger()->should_log<quill::LogLevel::TraceL3>()) [[unlikely]]
        log_t3(web, "Data hexdump\n{}", utils::hexdump(buf, length));

    // Make sure this connection isn't closed
    if (!conn->open()) [[unlikely]] {
        log_w(web, "Write callback for {} called after close().", conn->url());

        return size; // piped bytes to /dev/null
    }

    // Add data to write buf
    // NOLINTNEXTLINE(*-pointer-arithmetic)
    conn->write_buf_.insert(conn->write_buf_.end(), buf, buf + size);

    if (frame->bytesleft == 0) [[likely]] { // got all data in frame; most are short
        // log some data
        log_bt(web, "Entering user data callback for {}", conn->url());
        log_d(
            web,
            "Received {} bytes from {} over WS, entering user callback",
            conn->write_buf_.size(),
            conn->url()
        );

        // enter callback
        const auto start = std::chrono::steady_clock::now(); // start time
        conn->on_data_(conn, conn->write_buf_);              // callback
        const auto end = std::chrono::steady_clock::now();   // end time

        // done with callback, clear write buffer
        conn->write_buf_.clear();

        // Log information about callback
        const std::chrono::duration<double, std::milli> time_in_cb = end - start;
        log_d(web, "Time spent in callback: {}", time_in_cb);
    }

    // Return "bytes written"
    return size;
}

void
WebSocketConnection::start_()
{
    assert(ready());
    assert(!open());

    // Populate backtrace
    log_bt(web, "Setting up WS connection to {}", url());

    // Clear error buffer
    clear_error_buffer_();

    // Set url
    curl_easy_setopt(curl_handle(), CURLOPT_URL, url().c_str());

    // Set up write function
    curl_easy_setopt(curl_handle(), CURLOPT_WRITEFUNCTION, write_callback_);
    curl_easy_setopt(curl_handle(), CURLOPT_WRITEDATA, this);

    // Mark the websocket as open
    open() = true;
    log_d(web, "Set up web socket connection to {}", url());
}

size_t
WebSocketConnection::close(WebSocketCloseStatus status, std::vector<uint8_t> data)
{
    assert(ready());

    // Backtrace log
    log_bt(
        web,
        "Send WS close message to {} with status {} and data {}",
        url(),
        status,
        data
    );

    // Log about arguments
    log_i(web, "Closing WebSocket connection to {} with code {}", url(), status);
    log_d(web, "{} bytes of data provided", data.size());

    if ( //
        !data.empty()
        && logging::get_web_logger()->should_log<quill::LogLevel::TraceL2>()
    ) [[unlikely]] {
        log_t2(web, "Data hexdump\n{}", utils::hexdump(data));
    }

    // Make sure we're not already closed
    if (!open()) {
        log_w(
            web,
            "close() called on closed connection to {} (may have been dropped)",
            url()
        );

        return 0;
    }

    // Make status big endian
    // NOLINTBEGIN(*-union-access)
    union {
        uint16_t val;
        uint8_t bytes[2]; // NOLINT(*-c-arrays)
    } status_bytes{};

    status_bytes.val = static_cast<uint16_t>(status);

    if (std::endian::native == std::endian::little)
        std::swap(status_bytes.bytes[0], status_bytes.bytes[1]);

    // Add status bytes to data
    data.insert(data.begin(), status_bytes.bytes[1]);
    data.insert(data.begin(), status_bytes.bytes[0]);
    // NOLINTEND(*-union-access)

    // Mark as closed
    open() = false;

    // Send the data
    return send(std::move(data), CURLWS_CLOSE);
}

size_t
WebSocketConnection::send(std::vector<uint8_t> data, unsigned flags)
{
    assert(ready());

    // Populate backtrace
    log_bt(
        web,
        "Websocket send to {} with flags {:#b} and {} bytes: {}",
        url(),
        flags,
        data.size(),
        data
    );

    // Log about arguments
    log_t1(web, "Sending {} bytes to {} with flags {:#b}", data.size(), url(), flags);

    if (logging::get_web_logger()->should_log<quill::LogLevel::TraceL3>()) [[unlikely]]
        log_t3(web, "Data hexdump\n{}", utils::hexdump(data.data(), data.size()));

    // Clear error buffer
    clear_error_buffer_();

    // Send request
    size_t sent = 0;
    auto res = curl_ws_send(curl_handle(), data.data(), data.size(), &sent, 0, flags);

    // Handle any errors
    if (res != CURLE_OK)
        process_curl_error_(res);

    // Return bytes sent
    assert(sent == data.size());
    return sent;
}

} // namespace web
} // namespace raccoon
