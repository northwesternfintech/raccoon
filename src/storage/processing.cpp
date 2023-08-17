#include "processing.hpp"

#include "common.hpp"

namespace raccoon {
namespace storage {
void
DataProcessor::process_incoming_data_(const std::string& string_data)
{
    std::variant<OrderbookSnapshot, OrderbookUpdate, Match> data{};
    auto err = glz::read_json(data, string_data);

    if (err) {
        log_e(main, "Error parsing data: {}", glz::format_error(err, string_data));
        return;
    }

    if (std::holds_alternative<OrderbookUpdate>(data)) {
        const auto& latestUpdate = std::get<OrderbookUpdate>(data);
        prox_.process_incoming_update_(latestUpdate);
        prox_.ob_to_redis_(redis_, latestUpdate.product_id);
    }
    else if (std::holds_alternative<OrderbookSnapshot>(data)) {
        const auto& latestSnapshot = std::get<OrderbookSnapshot>(data);
        prox_.process_incoming_snapshot_(latestSnapshot);
        prox_.ob_to_redis_(redis_, latestSnapshot.product_id);
    }
    else {
        log_e(main, "Unknown data type");
    }
}

} // namespace storage
} // namespace raccoon
