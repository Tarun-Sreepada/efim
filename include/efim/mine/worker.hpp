#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <efim/io/dataset.hpp>
#include <efim/core/level.hpp>
#include <efim/io/pattern_writer.hpp>
#include <efim/mine/scheduler.hpp>
#include <efim/core/types.hpp>

namespace efim {

struct SearchParams {
	Utility min_util;
	int threads;
};

// Per-thread search state. Scratch arrays are indexed by item id and reset after each
// projection via the `touched` list.
class Worker {
public:
	Worker(const Dataset& data, const SearchParams& params, Scheduler& scheduler, PatternWriter& writer);

	// Pops and expands tasks until the scheduler runs dry.
	void run();
	// Writes out any buffered patterns.
	void flush();

	std::size_t pattern_count() const { return count_; }

private:
	// Subtrees are split into tasks for other threads only near the top of the search
	// and only when their database * extension count is large enough to be worth the copy.
	static constexpr int kMaxSplitDepth = 2;
	static constexpr std::uint64_t kSplitWork = std::uint64_t{1} << 20;
	static constexpr std::size_t kFlushBytes = std::size_t{1} << 20;

	// Per-item scratch, interleaved so each item touches one cache line.
	// `mark` = deepest depth on the current path at which the item is secondary, -1 if
	// none. The root (depth 0) keeps every item, so it is never marked.
	struct alignas(32) Scratch {
		Utility local = 0, subtree = 0;
		int mark = -1;
		int seen = 0;
	};

	void execute(const Task& task);
	Level& level_at(std::size_t depth);
	// Projects `db` (the database at `depth`) on db.primary[k], records the pattern
	// and recurses or hands the subtree to other threads.
	void expand(const Level& db, int depth, std::size_t k);
	// Appends "item\titem...:utility\n" for the current prefix.
	void emit(Utility utility);

	const Dataset& data_;
	SearchParams params_;
	Scheduler& scheduler_;
	PatternWriter& writer_;

	std::vector<Scratch> st_;
	std::vector<std::int32_t> slot_;
	std::vector<Item> touched_, prefix_items_, new_key_;
	std::vector<Utility> new_val_;
	std::vector<std::unique_ptr<Level>> levels_;  // levels_[d] = reusable database at depth d
	std::string out_buf_;                         // formatted patterns not yet written
	std::size_t count_ = 0;
};

}  // namespace efim
