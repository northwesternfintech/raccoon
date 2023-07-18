#include "common.hpp"
#include "product_tracker.hpp"

#include <glaze/glaze.hpp>
#include <hiredis/hiredis.h>

namespace raccoon {
namespace storage {

struct OrderbookSnapshot {
    std::string time;
    std::string type = "snapshot";
    std::string product_id;
    std::vector<std::tuple<std::string, std::string>> asks;
    std::vector<std::tuple<std::string, std::string>> bids;
};

struct Update {
    std::string time;
    std::string type = "l2update";
    std::string product_id;
    std::vector<std::tuple<std::string, std::string, std::string>> changes;
};

class OrderbookProcessor {
private:
    std::unordered_map<std::string, ProductTracker> Orderbook;
    redisContext* redis;

public:
    OrderbookProcessor(redisContext* c) { redis = c; }

    void process_incoming_data(const std::string& string_data);

private:
    void process_incoming_snapshot(const ObSnapshot& newOb);
    void process_incoming_update(const Update& newUpdate);
    void ob_to_redis(std::string product_id);
    void map_to_redis(
        const std::unordered_map<double, double>& table, const std::string& map_id
    );
};

} // namespace storage
} // namespace raccoon

template <>
struct glz::meta<raccoon::storage::ObSnapshot> {
    using T = raccoon::storage::ObSnapshot;
    static constexpr auto value = object(
        "time",
        &T::time,
        "type",
        &T::type,
        "product_id",
        &T::product_id,
        "asks",
        &T::asks,
        "bids",
        &T::bids
    );
};

template <>
struct glz::meta<raccoon::storage::Update> {
    using T = raccoon::storage::Update;
    static constexpr auto value = object(
        "time",
        &T::time,
        "type",
        &T::type,
        "product_id",
        &T::product_id,
        "changes",
        &T::changes
    );
};
