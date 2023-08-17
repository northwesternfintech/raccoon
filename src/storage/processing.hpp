#include "common.hpp"
#include "orderbook.hpp"
#include "trades.hpp"

#include <glaze/glaze.hpp>
#include <hiredis/hiredis.h>

namespace raccoon {
namespace storage {

class DataProcessor {
public:
    DataProcessor(redisContext* c) { redis_ = c, prox_ = OrderbookProcessor(); }

    void process_incoming_data_(const std::string& data);

private:
    redisContext* redis_;
    OrderbookProcessor prox_;
};

} // namespace storage
} // namespace raccoon
