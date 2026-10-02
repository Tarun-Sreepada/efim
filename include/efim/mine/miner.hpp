#pragma once

#include <cstddef>
#include <string>

#include <efim/core/types.hpp>

namespace efim {

struct MineOptions {
	std::string input;
	std::string output;
	Utility min_util = 0;
	std::string sep = "\t";
	int threads = 0;  // 0 = all hardware threads; otherwise clamped to [1, hardware threads]
};

struct MineResult {
	std::size_t patterns = 0;
	std::size_t peak_rss = 0;  // bytes
	double seconds = 0;        // load + mine + write
	double load_seconds = 0;   // part of `seconds` spent reading and preprocessing the input
	int threads = 0;           // threads actually used
};

// EFIM high-utility itemset mining (Zida et al., Knowl Inf Syst 51, 2017). Streams every
// pattern to `opt.output` as `item<TAB>item...:utility`; with more than one thread the
// line order is nondeterministic.
MineResult mine(const MineOptions& opt);

}  // namespace efim
