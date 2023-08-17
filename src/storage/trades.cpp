#include "trades.hpp"

namespace raccoon {
namespace storage {
void
TradeProcessor::process_incoming_match(const Match& match)
{
    if (last_reset_ + std::chrono::seconds(1) < std::chrono::system_clock::now()) {
        matches_.clear();
        last_reset_ = std::chrono::system_clock::now();
    }
    matches_.push_back(match);
}

void
TradeProcessor::matches_to_redis(redisContext* redis)
{
    std::string serialized_matches;
    glz::write_json(matches_, serialized_matches);

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(redis, "SET %s %s", "matches", serialized_matches.c_str())
    );

    if (reply == nullptr) {
        log_e(main, "Redis error: {}", redis->errstr);
        return;
    }
}

} // namespace storage
} // namespace raccoon
