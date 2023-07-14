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

    if (!s) {
        log_d(main, "ERROR PARSING");
    }

    if (std::holds_alternative<Update>(data)) {
        Update LatestUpdate = std::get<Update>(data);
        log_i(main, "Received update for product_id {}", LatestUpdate.product_id);
    }
    else {
      ObSnapshot LatestSnapshot = std::get<ObSnapshot>(data);
        log_i(main, "Received snapshot for product_id {}", LatestSnapshot.product_id);
    }
}
} // namespace storage
} // namespace raccoon
