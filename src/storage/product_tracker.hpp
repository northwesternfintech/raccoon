#include "common.hpp"

namespace raccoon {
namespace storage {
struct ProductTracker {
    std::unordered_map<double, int> bids;
    std::unordered_map<double, int> asks;
};
} // namespace storage
} // namespace raccoon
