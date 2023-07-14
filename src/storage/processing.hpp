#include "common.hpp"

#include <glaze/glaze.hpp>

namespace raccoon {
namespace storage {
struct Order {
    double price;
    std::string size;
};

struct ObSnapshot {
    std::string time;
    std::string type;
    std::string product_id;
    std::vector<Order> asks;
};

struct Change {
    std::string action;
    double price;
    std::string size;
};

struct Update {
    std::string time;
    std::string type;
    std::string product_id;
    std::vector<Change> changes;
};

class OrderbookProcessor {
public:
    void process_incoming_data(std::string string_data);
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
        &T::asks
    );
};

template <>
struct glz::meta<raccoon::storage::Change> {
    using T = raccoon::storage::Change;
    static constexpr auto value =
        object("action", &T::action, "price", &T::price, "size", &T::size);
};

template <>
struct glz::meta<raccoon::storage::Order> {
    using T = raccoon::storage::Order;
    static constexpr auto value = object("price", &T::price, "size", &T::size);
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
