#include "processing.hpp"

#include "common.hpp"

namespace raccoon {
namespace storage {
void
DataProcessor::process_incoming_data(const std::string& string_data)
{
    std::variant<OrderbookSnapshot, OrderbookUpdate, Match> data{};
    auto err = glz::read_json(data, string_data);

    if (err) {
        log_e(main, "Error parsing data: {}", glz::format_error(err, string_data));
        return;
    }

    if (std::holds_alternative<OrderbookUpdate>(data)) {
        const OrderbookUpdate& latestUpdate = std::get<OrderbookUpdate>(data);
        orderbook_prox_.process_incoming_update(latestUpdate);
        orderbook_prox_.ob_to_redis(redis_, latestUpdate.product_id);
    }
    else if (std::holds_alternative<OrderbookSnapshot>(data)) {
        const OrderbookSnapshot& latestSnapshot = std::get<OrderbookSnapshot>(data);
        orderbook_prox_.process_incoming_snapshot(latestSnapshot);
        orderbook_prox_.ob_to_redis(redis_, latestSnapshot.product_id);
    }
    else if (std::holds_alternative<Match>(data)) {
        const Match& latestMatch = std::get<Match>(data);
        trade_prox_.process_incoming_match(latestMatch);
        trade_prox_.matches_to_redis(redis_);
    }
    else {
        log_e(main, "Unknown data type");
    }
}

} // namespace storage
} // namespace raccoon
