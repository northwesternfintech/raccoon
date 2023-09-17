#include "common.hpp"
#include "orderbook.hpp"
#include "trades.hpp"

#include <glaze/glaze.hpp>
#include <hiredis/hiredis.h>

namespace raccoon {
namespace storage {

class DataProcessor {
    redisContext* redis_;
    OrderbookProcessor orderbook_prox_;
    TradeProcessor trade_prox_;

public:
    explicit DataProcessor(redisContext* redis) : redis_(redis), orderbook_prox_() {}

    template <Container C>
    void
    process_incoming_data(const C& json_data)
    {
        std::string str(json_data.begin(), json_data.end());
        process_incoming_data(str);
    }

    void process_incoming_data(const std::string& json_data);
};

} // namespace storage
} // namespace raccoon
