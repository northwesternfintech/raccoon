#include "common.hpp"

namespace raccoon {
namespace storage {
struct ProductTracker {
    std::map<double, double> bids;
    std::map<double, double> asks;
};
} // namespace storage
} // namespace raccoon
