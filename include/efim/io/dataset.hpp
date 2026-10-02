#pragma once

#include <memory>
#include <string>
#include <vector>

#include <efim/core/level.hpp>
#include <efim/core/types.hpp>

namespace efim {

// The preprocessed input: items with TWU < minUtil removed, renamed so a larger id
// means a larger TWU, transactions sorted and merged into `root`.
struct Dataset {
	std::shared_ptr<Level> root;          // primary items and occurrence index already built
	std::vector<std::string> rename;      // rename[id] = original item name; index 0 unused
	std::vector<std::string> names_tab;   // "\t" + name, so a pattern is a concatenation
	std::size_t num_items = 0;
	std::size_t num_transactions = 0;     // before merging identical ones

	// Items/utilities are split by `sep`, one transaction per line: `a c d:8:5 1 2`.
	static Dataset load(const std::string& path, const std::string& sep, Utility min_util);
};

}  // namespace efim
