#include "ws.hpp"

#include "common.hpp"

#include <curl/curl.h>

namespace raccoon {
namespace web {

size_t
WebSocketConnection::write_callback_(
    uint8_t* buf, size_t elem_size, size_t length, void* user_data
)
{
    // Get our connection
    auto* conn = static_cast<WebSocketConnection*>(user_data);
    log_t1(web, "Callback ran for {}", conn->url());

    assert(conn->ready());

    // Compute size
    size_t size = elem_size * length;

    // Make sure this connection isn't closed
    if (!conn->open()) {
        log_w(web, "Write callback called after close().");

        return size; // piped bytes to /dev/null
    }

    // Get our frame
    const auto* frame = curl_ws_meta(conn->curl_handle());

    // Add data to write buf
    // NOLINTNEXTLINE(*-pointer-arithmetic)
    conn->write_buf_.insert(conn->write_buf_.end(), buf, buf + size);

    if (frame->bytesleft == 0) { // got all data in frame
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

    log_d(libcurl, "Starting web socket connection to {}", url());

    // Clear error buffer
    clear_error_buffer_();

    // Set url and write function
    curl_easy_setopt(curl_handle(), CURLOPT_URL, url().c_str());
    curl_easy_setopt(curl_handle(), CURLOPT_WRITEFUNCTION, write_callback_);

    // Pass class instance to callback
    curl_easy_setopt(curl_handle(), CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_handle(), CURLOPT_PRIVATE, this);

    // Mark the websocket as open
    open() = true;
}

size_t
WebSocketConnection::close(WebSocketCloseStatus status, std::vector<uint8_t> data)
{
    assert(ready());

    if (!open())
        return 0;

    log_i(web, "Closing WebSocket connection to {}", url());

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
