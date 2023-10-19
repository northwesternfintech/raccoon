#include "processing.hpp"

#include "common.hpp"

namespace raccoon {
namespace storage {

void
DataProcessor::process_incoming_data(const std::string& json_data)
{
    // Parse JSON data using glaze
    std::variant<OrderbookSnapshot, OrderbookUpdate, Match> data{};

    auto err = glz::read_json(data, json_data);
    if (err) [[unlikely]] {
        log_e(main, "Error parsing data: {}", glz::format_error(err, json_data));
        return;
    }

    auto processMessage = [&](const auto& message) {
        using T = std::decay_t<decltype(data)>;
        // Check what kind of data we got
        if constexpr (std::is_same_v<T, OrderbookUpdate>) {
            // We got an update
            orderbook_prox_.process_incoming_update(message);
            orderbook_prox_.ob_to_redis(redis_, message.product_id);
        }
        else if constexpr (std::is_same_v<T, OrderbookSnapshot>) {
            // We got a snapshot

            orderbook_prox_.process_incoming_snapshot(message);
            orderbook_prox_.ob_to_redis(redis_, message.product_id);
        }
        else if constexpr (std::is_same_v<T, Match>) {
            // We got a match
            const auto& match = std::get<Match>(data);

            trade_prox_.process_incoming_match(match);
            trade_prox_.matches_to_redis(redis_);
        }
        else {
            log_e(main, "Unknown data type");
        }
    };

    std::visit(processMessage, data);
}

} // namespace storage
} // namespace raccoon
