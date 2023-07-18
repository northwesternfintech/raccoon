#include "common.hpp"
#include "storage.hpp"

#include <glaze/glaze.hpp>
#include <quill/LogLevel.h>

namespace raccoon {
namespace storage {

void
OrderbookProcessor::process_incoming_data(const std::string& string_data)
{
    std::variant<OrderbookSnapshot, OrderbookUpdate> data{};
    auto err = glz::read_json(data, string_data);

    if (err) {
        log_e(main, "Error parsing data: {}", glz::format_error(err, string_data));
        return;
    }

    if (std::holds_alternative<OrderbookUpdate>(data)) {
        const auto& latestUpdate = std::get<OrderbookUpdate>(data);
        process_incoming_update_(latestUpdate);
        ob_to_redis_(latestUpdate.product_id);
    }
    else {
        const auto& latestSnapshot = std::get<OrderbookSnapshot>(data);
        process_incoming_snapshot_(latestSnapshot);
        ob_to_redis_(latestSnapshot.product_id);
    }
}

void
OrderbookProcessor::ob_to_redis_(const std::string& product_id)
{
    log_d(main, "Pushing orderbook {} to redis", product_id);

    product_tracker tracker = orderbook_[product_id];
    map_to_redis_(tracker.asks, product_id + "-ASKS");
    map_to_redis_(tracker.bids, product_id + "-BIDS");
}

void
OrderbookProcessor::process_incoming_update_(const OrderbookUpdate& newUpdate)
{
    log_d(main, "Processing incoming update for {}", newUpdate.product_id);

    auto [it, inserted] = orderbook_.emplace(newUpdate.product_id, product_tracker());
    product_tracker& tracker = it->second;

    auto updateOrderbook =
        [](std::unordered_map<double, double>& orderSide, double price, double volume) {
            if (volume <= 0.00000000001) {
                orderSide.erase(price);
            }
            else {
                auto [iterator, insert] = orderSide.emplace(price, volume);
                if (!insert) {
                    iterator->second += volume;
                }
            }
        };

    for (const auto& change : newUpdate.changes) {
        bool isBuy = std::get<0>(change) == "BUY";
        double price = std::stod(std::get<1>(change));
        double volume = std::stod(std::get<2>(change));

        updateOrderbook(isBuy ? tracker.bids : tracker.asks, price, volume);
    }
}

void
OrderbookProcessor::process_incoming_snapshot_(const OrderbookSnapshot& newOb)
{
    log_d(main, "Processing incoming snapshot for {}", newOb.product_id);

    auto [it, inserted] = orderbook_.emplace(newOb.product_id, product_tracker());
    product_tracker& tracker = it->second;

    auto updateSnapshot =
        [](std::unordered_map<double, double>& orderSide,
           const std::vector<std::tuple<std::string, std::string>>& orders) {
            for (const auto& order : orders) {
                double price = std::stod(std::get<0>(order));
                double volume = std::stod(std::get<1>(order));
                orderSide[price] = volume;
            }
        };

    updateSnapshot(tracker.asks, newOb.asks);
    updateSnapshot(tracker.bids, newOb.bids);
}

void
OrderbookProcessor::map_to_redis_(
    const std::unordered_map<double, double>& table, const std::string& map_id
)
{
    std::vector<const char*> argv;
    argv.reserve(
        2 * table.size() + 2
    );

    argv.push_back("HMSET");
    argv.push_back(map_id.c_str());

    std::vector<std::string> kvPairs;
    kvPairs.reserve(2 * table.size());

    for (const auto& pair : table) {
        kvPairs.push_back(std::to_string(pair.first));
        kvPairs.push_back(std::to_string(pair.second));
        argv.push_back(kvPairs[kvPairs.size() - 2].c_str());
        argv.push_back(kvPairs[kvPairs.size() - 1].c_str());
    }

    auto reply = static_cast<redisReply*>(
        redisCommandArgv(redis_, static_cast<int>(argv.size()), argv.data(), NULL)
    );

    if (reply == NULL) {
        log_e(main, "Error: %s\n", redis_->errstr);
        return;
    }

    freeReplyObject(reply);
}
} // namespace storage
} // namespace raccoon
