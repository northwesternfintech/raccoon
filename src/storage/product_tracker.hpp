#include "common.hpp"

namespace raccoon {
namespace storage {
struct ProductTracker {
    std::unordered_map<double, double> bids;
    std::unordered_map<double, double> asks;
};
} // namespace storage
} // namespace raccoon
