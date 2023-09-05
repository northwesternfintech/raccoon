#include "ws.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "utils/utils.hpp"

#include <curl/curl.h>
#include <quill/LogLevel.h>

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
    assert(frame);

    // Log information
    log_t1(
        web,
        "WebSocket data callback ran for {} ({} bytes, {} left)",
        conn->url(),
        size,
        frame->bytesleft
    );

    if (logging::get_web_logger()->should_log<quill::LogLevel::TraceL3>())
        log_t3(web, "Data hexdump\n{}", utils::hexdump(buf, length));

    log_bt(
        web,
        "WS write callback for url {} with buf of len {} ({}x{}), {} bytes left",
        conn->url(),
        size,
        elem_size,
        length,
        frame->bytesleft
    );

    // Make sure this connection isn't closed
    if (!conn->open()) {
        log_w(web, "Write callback called after close().");

        return size; // piped bytes to /dev/null
    }

    // Add data to write buf
    // NOLINTNEXTLINE(*-pointer-arithmetic)
    conn->write_buf_.insert(conn->write_buf_.end(), buf, buf + size);

    if (frame->bytesleft == 0) { // got all data in frame
        log_bt(web, "Entering user data callback for {}", conn->url());

        conn->on_data_(conn, conn->write_buf_);
        conn->write_buf_.clear(); // done, clear write buffer
    }

    // Return "bytes written"
    return size;
}

void
WebSocketConnection::start_()
{
    assert(ready());
    assert(!open());

    log_d(web, "Starting web socket connection to {}", url());

    // Clear error buffer
    clear_error_buffer_();

    // Set url
    curl_easy_setopt(curl_handle(), CURLOPT_URL, url().c_str());

    // Set up write function
    curl_easy_setopt(curl_handle(), CURLOPT_WRITEFUNCTION, write_callback_);
    curl_easy_setopt(curl_handle(), CURLOPT_WRITEDATA, this);

    // Mark the websocket as open
    log_bt(web, "Set up WS connection to {}", url());
    open() = true;
}

size_t
WebSocketConnection::close(WebSocketCloseStatus status, std::vector<uint8_t> data)
{
    assert(ready());

    log_d(web, "Closing WebSocket connection to {} with code {}", url(), status);
    log_t2(web, "Data: {}", data);

    log_bt(
        web,
        "Send WS close message to {} with status {} and data {}",
        url(),
        status,
        data
    );

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

    log_t1(web, "Sending {} bytes to {} with flags {:#b}", data.size(), url(), flags);

    if (logging::get_web_logger()->should_log<quill::LogLevel::TraceL3>())
        log_t3(web, "Hexdump\n{}", utils::hexdump(data.data(), data.size()));

    log_bt(
        web,
        "Websocket send to {} with flags {:#b} and {} bytes: {}",
        url(),
        flags,
        data.size(),
        data
    );

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
