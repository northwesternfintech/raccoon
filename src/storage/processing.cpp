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

    std::string product_id;

    if (std::holds_alternative<Update>(data)) {
        Update latestUpdate = std::get<Update>(data);
        product_id = latestUpdate.product_id;
        process_incoming_update(latestUpdate);
    }
    else {
        ObSnapshot latestSnapshot = std::get<ObSnapshot>(data);
        process_incoming_snapshot(latestSnapshot);
        product_id = latestSnapshot.product_id;
    }

    ob_to_redis(product_id);
}

void
OrderbookProcessor::process_incoming_snapshot(ObSnapshot newOb)
{
    log_d(main, "Processing incoming snapshot for {}", newOb.product_id);

    auto insertion_result = Orderbook.insert({newOb.product_id, ProductTracker()});
    ProductTracker tracker = insertion_result.first->second;

    for (auto ask = newOb.asks.begin(); ask != newOb.asks.end(); ++ask) {
        double price = std::stod(std::get<0>(*ask));
        double volume = std::stod(std::get<1>(*ask));
        tracker.asks[price] = volume;
    };

    for (auto bid = newOb.bids.begin(); bid != newOb.bids.end(); ++bid) {
        double price = std::stod(std::get<0>(*bid));
        double volume = std::stod(std::get<1>(*bid));
        tracker.bids[price] = volume;
    };

    Orderbook[newOb.product_id] = tracker;
}

void
OrderbookProcessor::process_incoming_update(Update newUpdate)
{
    log_d(main, "Processing incoming update for {}", newUpdate.product_id);

    auto insertion_result = Orderbook.insert({newUpdate.product_id, ProductTracker()});
    ProductTracker tracker = insertion_result.first->second;

    for (auto change = newUpdate.changes.begin(); change != newUpdate.changes.end();
         ++change) {
        bool isBuy = std::get<0>(*change) == "BUY";
        double price = std::stod(std::get<1>(*change));
        double volume = std::stod(std::get<2>(*change));
        if (volume == 0.0) {
            if (isBuy) {
                tracker.bids.erase(price);
            }
            else {
                tracker.asks.erase(price);
            }
        }
        else {
            if (isBuy) {
                if (tracker.bids.find(price) != tracker.bids.end()) {
                    tracker.bids[price] += volume;
                }
                else {
                    tracker.bids[price] = volume;
                }
            }
            else {
                if (tracker.asks.find(price) != tracker.asks.end()) {
                    tracker.asks[price] += volume;
                }
                else {
                    tracker.asks[price] = volume;
                }
            }
        }
    };
    Orderbook[newUpdate.product_id] = tracker;
}

void
OrderbookProcessor::ob_to_redis(std::string product_id)
{
    log_d(main, "Pushing orderbook {} to redis", product_id);

    ProductTracker tracker = Orderbook[product_id];
    map_to_redis(tracker.asks, product_id + "-ASKS");
    map_to_redis(tracker.bids, product_id + "-BIDS");
}

void
OrderbookProcessor::map_to_redis(std::map<double, double> table, std::string map_id)
{
    std::vector<std::string> kvPairs;
    for (auto const& pair : table) {
        kvPairs.push_back(std::to_string(pair.first));
        kvPairs.push_back(std::to_string(pair.second));
    }

    std::vector<const char*> argv;
    argv.push_back("HMSET");
    argv.push_back(map_id.c_str());

    for (const std::string& s : kvPairs) {
        argv.push_back(s.c_str());
    }

    redisReply* reply =
        (redisReply*)redisCommandArgv(redis, argv.size(), &(argv[0]), NULL);

    if (reply == NULL) {
        log_e(main, "Error: %s\n", redis->errstr);
    }
    else {
        freeReplyObject(reply);
    }
}

} // namespace storage
} // namespace raccoon
