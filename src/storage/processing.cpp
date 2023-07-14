#include "common.hpp"
#include "storage.hpp"

#include <glaze/glaze.hpp>
#include <quill/LogLevel.h>

namespace raccoon {
namespace storage {
void
OrderbookProcessor::process_incoming_data(std::string string_data)
{
    std::variant<ObSnapshot, Update> data{};
    auto s = glz::read_json(data, string_data);

    if (s) {
        std::string error = glz::format_error(s, string_data);
        log_e(main, "Error parsing data: {}", error);
        return;
    }

    if (std::holds_alternative<Update>(data)) {
        Update latestUpdate = std::get<Update>(data);
        process_incoming_update(latestUpdate);
    }
    else {
        ObSnapshot LatestSnapshot = std::get<ObSnapshot>(data);
        // process_incoming_snapshot(LatestSnapshot);
    }
}

void
OrderbookProcessor::process_incoming_snapshot(ObSnapshot newOb)
{
    auto insertion_result = Orderbook.insert({newOb.product_id, ProductTracker()});

    ProductTracker tracker = insertion_result.first->second;

    // Shouldn't remove while iterating
    std::vector<double> asks_to_erase;
    std::vector<double> bids_to_erase;

    // for (auto ask = newOb.asks.begin(); ask != newOb.asks.end(); ++ask) {
    // if (ask->size == 0) {
    // asks_to_erase.push(ask->price);
    // }
    // }
}

void
OrderbookProcessor::process_incoming_update(Update newUpdate)
{
    log_d(main, "Processing incoming update for {}", newUpdate.product_id);

    auto insertion_result = Orderbook.insert({newUpdate.product_id, ProductTracker()});
    ProductTracker tracker = insertion_result.first->second;

    std::vector<double> sells_to_erase;
    std::vector<double> buys_to_erase;

    for (auto change = newUpdate.changes.begin(); change != newUpdate.changes.end();
         ++change) {};
}

} // namespace storage
} // namespace raccoon
