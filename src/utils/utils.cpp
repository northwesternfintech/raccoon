#include "utils.hpp"

namespace raccoon {
namespace utils {

std::string
hexdump(const void* data, size_t size)
{
    constexpr size_t WIDTH = 0x10;

    std::string res;
    const char* ptr = static_cast<const char*>(data);

    // Print header
    res += fmt::format("{:010d} bytes ({:#08x})\n", size, size);

    // Print body
    for (size_t i = 0; i < size; i += WIDTH) {
        res += fmt::format("{:04x}:  ", i);

        /* show hex to the left */
        for (size_t j = 0; j < WIDTH; j++) {
            if (i + j < size)
                res += fmt::format("{:02x} ", ptr[i + j]); // NOLINT(*-arithmetic)
            else
                res += "   ";
        }

        /* show data on the right */
        res += "    ";

        for (size_t j = 0; (j < WIDTH) && (i + j < size); j++) {
            char c = ptr[i + j]; // NOLINT(*-arithmetic)
            res += isprint(c) ? c : '.';
        }

        res += "\n";
    }

    return res;
}

} // namespace utils
} // namespace raccoon
