#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <efim/core/types.hpp>

namespace efim {

// A transaction of a (projected) database, items sorted ascending. A view points into
// an ancestor's storage; an owned transaction lives in its own Level's storage.
struct Transaction {
	const Item* items;
	const Utility* utils;
	std::uint32_t len;
	Utility prefix;    // utility of the current prefix itemset in this transaction
	std::int64_t own;  // offset into Level::items/utils, or -1 for a view
};

struct Occurrence {
	std::uint32_t tid;
	std::uint32_t pos;
};

// A (projected) database plus the occurrence index used to project it further.
struct Level {
	std::vector<Item> items;  // storage for owned transactions
	std::vector<Utility> utils;
	std::vector<Transaction> txs;
	std::vector<Item> primary;    // items to extend with (subtree utility >= minUtil), ascending
	std::vector<Item> secondary;  // items kept in deeper projections (local utility >= minUtil)
	std::vector<std::uint32_t> occ_begin;  // primary[k] occurs at occ[occ_begin[k], occ_begin[k + 1])
	std::vector<Occurrence> occ;
	std::vector<std::uint32_t> cursor;  // scratch for index()

	void clear();

	const Item* items_of(const Transaction& t) const { return t.own >= 0 ? items.data() + t.own : t.items; }

	// Appends a transaction, merging it into the previous one when their items are
	// equal. A view must point outside this Level; otherwise the data is copied.
	void add(const Item* it, const Utility* ut, std::uint32_t n, Utility prefix, bool view);

	// Turns owned offsets into pointers; call once every transaction has been added.
	void finalize();

	// Builds the occurrence index for `primary`. `slot` is indexed by item and must be
	// all -1; it is left that way.
	void index(std::vector<std::int32_t>& slot);

	// A self-contained copy whose transactions all point into its own storage.
	std::shared_ptr<Level> materialize() const;
};

}  // namespace efim
