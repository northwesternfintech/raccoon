#include "common.hpp"

namespace raccoon {
namespace storage {
struct Match {
    std::string type = "match";
    std::string time;
    int trade_id;
    std::string maker_order_id;
    std::string taker_order_id;
    std::string side;
    std::string size;
    std::string price;
    std::string product_id;
    int sequence;
};

} // namespace storage
} // namespace raccoon

template <>
struct glz::meta<raccoon::storage::Match> {
    using T = raccoon::storage::Match;
    static constexpr auto value = object(
        "type",
        &T::type,
        "time",
        &T::time,
        "trade_id",
        &T::trade_id,
        "maker_order_id",
        &T::maker_order_id,
        "taker_order_id",
        &T::taker_order_id,
        "side",
        &T::side,
        "size",
        &T::size,
        "price",
        &T::price,
        "product_id",
        &T::product_id,
        "sequence",
        &T::sequence
    );
};
