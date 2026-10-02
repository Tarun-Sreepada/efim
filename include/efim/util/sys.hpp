#pragma once

#include <cstddef>

namespace efim {

// Peak resident set size of this process in bytes (0 if unavailable).
std::size_t peak_rss_bytes();

// Hardware thread count; hardware_concurrency() may return 0 when unknown.
int hardware_threads();

}  // namespace efim
