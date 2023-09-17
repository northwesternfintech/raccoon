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

    // Check what kind of data we got
    if (std::holds_alternative<OrderbookUpdate>(data)) {
        // We got an update
        const auto& update = std::get<OrderbookUpdate>(data);

        orderbook_prox_.process_incoming_update(update);
        orderbook_prox_.ob_to_redis(redis_, update.product_id);
    }
    else if (std::holds_alternative<OrderbookSnapshot>(data)) [[unlikely]] {
        // We got a snapshot
        const auto& snapshot = std::get<OrderbookSnapshot>(data);

        orderbook_prox_.process_incoming_snapshot(snapshot);
        orderbook_prox_.ob_to_redis(redis_, snapshot.product_id);
    }
    else if (std::holds_alternative<Match>(data)) {
        // We got a match
        const auto& match = std::get<Match>(data);

        trade_prox_.process_incoming_match(match);
        trade_prox_.matches_to_redis(redis_);
    }
    else [[unlikely]] {
        log_e(main, "Unknown data type");
    }
}

} // namespace storage
} // namespace raccoon
