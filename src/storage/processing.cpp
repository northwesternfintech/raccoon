#include "common.hpp"
#include "storage.hpp"

#include <glaze/glaze.hpp>
#include <quill/LogLevel.h>

namespace raccoon {
namespace storage {

void
OrderbookProcessor::process_incoming_data(const std::string& string_data)
{
    std::variant<ObSnapshot, Update> data{};
    auto s = glz::read_json(data, string_data);

    if (s) {
        log_e(main, "Error parsing data: {}", glz::format_error(s, string_data));
        return;
    }

    if (std::holds_alternative<Update>(data)) {
        const auto& latestUpdate = std::get<Update>(data);
        process_incoming_update(latestUpdate);
        ob_to_redis(latestUpdate.product_id);
    }
    else {
        const auto& latestSnapshot = std::get<ObSnapshot>(data);
        process_incoming_snapshot(latestSnapshot);
        ob_to_redis(latestSnapshot.product_id);
    }
}

void
OrderbookProcessor::ob_to_redis(const std::string& product_id)
{
    log_d(main, "Pushing orderbook {} to redis", product_id);

    ProductTracker tracker = Orderbook[product_id];
    map_to_redis(tracker.asks, product_id + "-ASKS");
    map_to_redis(tracker.bids, product_id + "-BIDS");
}

void
OrderbookProcessor::process_incoming_update(const Update& newUpdate)
{
    log_d(main, "Processing incoming update for {}", newUpdate.product_id);

    auto [it, inserted] = Orderbook.emplace(newUpdate.product_id, ProductTracker());
    ProductTracker& tracker = it->second;

    auto updateOrderbook =
        [](std::unordered_map<double, double>& orderSide, double price, double volume) {
            if (volume == 0.0) {
                orderSide.erase(price);
            }
            else {
                auto [it, inserted] = orderSide.emplace(price, volume);
                if (!inserted) {
                    it->second += volume;
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
OrderbookProcessor::process_incoming_snapshot(const ObSnapshot& newOb)
{
    log_d(main, "Processing incoming snapshot for {}", newOb.product_id);

    auto [it, inserted] = Orderbook.emplace(newOb.product_id, ProductTracker());
    ProductTracker& tracker = it->second;

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
OrderbookProcessor::map_to_redis(
    const std::unordered_map<double, double>& table, const std::string& map_id
)
{
    std::vector<const char*> argv = {"HMSET", map_id.c_str()};

    for (const auto& [key, value] : table) {
        argv.emplace_back(std::to_string(key).c_str());
        argv.emplace_back(std::to_string(value).c_str());
    }

    auto reply = static_cast<redisReply*>(
        redisCommandArgv(redis, static_cast<int>(argv.size()), argv.data(), nullptr)
    );

    if (reply == nullptr) {
        log_e(main, "Error: %s\n", redis->errstr);
        return;
    }

    freeReplyObject(reply);
}

} // namespace storage
} // namespace raccoon
