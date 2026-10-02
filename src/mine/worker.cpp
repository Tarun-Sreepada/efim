#include <efim/mine/worker.hpp>

#include <algorithm>
#include <charconv>

namespace efim {

Worker::Worker(const Dataset& data, const SearchParams& params, Scheduler& scheduler, PatternWriter& writer)
	: data_(data), params_(params), scheduler_(scheduler), writer_(writer), st_(data.rename.size()),
	  slot_(data.rename.size(), -1) {}

void Worker::flush() {
	if (out_buf_.empty()) return;
	writer_.write(out_buf_);
	out_buf_.clear();
}

void Worker::emit(Utility utility) {
	const auto& names = data_.names_tab;
	for (std::size_t i = 0; i < prefix_items_.size(); ++i)
		out_buf_.append(names[prefix_items_[i]], i ? 0 : 1, std::string::npos);  // skip the first tab
	char tmp[24];
	tmp[0] = ':';
	const auto r = std::to_chars(tmp + 1, tmp + 23, utility);
	*r.ptr = '\n';
	out_buf_.append(tmp, r.ptr + 1 - tmp);
	++count_;
	if (out_buf_.size() >= kFlushBytes) flush();
}

void Worker::run() {
	Task task;
	while (scheduler_.pop(task)) {
		execute(task);
		task = {};  // release the database before reporting completion
		scheduler_.done();
	}
}

void Worker::execute(const Task& task) {
	prefix_items_ = task.prefix;
	if (task.depth > 0)
		for (const Item x : task.level->secondary) st_[x].mark = task.depth;
	expand(*task.level, task.depth, task.slot);
	if (task.depth > 0)
		for (const Item x : task.level->secondary) st_[x].mark = -1;
}

Level& Worker::level_at(std::size_t depth) {
	while (levels_.size() <= depth) levels_.push_back(std::make_unique<Level>());
	return *levels_[depth];
}

void Worker::expand(const Level& db, int depth, std::size_t k) {
	const Item item = db.primary[k];
	const Utility min_util = params_.min_util;
	Level& child = level_at(depth + 1);
	child.clear();
	Utility utility = 0;

	for (std::uint32_t o = db.occ_begin[k]; o < db.occ_begin[k + 1]; ++o) {
		const Transaction& t = db.txs[db.occ[o].tid];
		const std::uint32_t pos = db.occ[o].pos;
		const Utility curr = t.utils[pos] + t.prefix;
		utility += curr;
		if (pos + 1 == t.len) continue;

		const Item* items = t.items + pos + 1;
		const Utility* utils = t.utils + pos + 1;
		const std::uint32_t n = t.len - pos - 1;
		Utility s = 0;
		std::uint32_t kept = 0;
		for (std::uint32_t i = 0; i < n; ++i) {
			if (depth > 0 && st_[items[i]].mark < depth) continue;
			s += utils[i];
			++kept;
		}
		if (kept == 0) continue;

		const bool view = kept == n;
		if (!view) {
			new_key_.clear();
			new_val_.clear();
		}
		Utility temp = 0;
		for (std::uint32_t i = 0; i < n; ++i) {
			const Item x = items[i];
			if (depth > 0 && st_[x].mark < depth) continue;
			if (!st_[x].seen) {
				st_[x].seen = 1;
				touched_.push_back(x);
			}
			st_[x].local += s + curr;
			st_[x].subtree += s + curr - temp;
			temp += utils[i];
			if (!view) {
				new_key_.push_back(x);
				new_val_.push_back(utils[i]);
			}
		}
		if (view)
			child.add(items, utils, n, curr, true);
		else
			child.add(new_key_.data(), new_val_.data(), kept, curr, false);
	}
	child.finalize();

	prefix_items_.push_back(item);
	if (utility >= min_util) emit(utility);

	for (const Item x : touched_) {
		if (st_[x].subtree >= min_util) child.primary.push_back(x);
		if (st_[x].local >= min_util) child.secondary.push_back(x);
		st_[x].local = st_[x].subtree = 0;
		st_[x].seen = 0;
	}
	touched_.clear();

	if (!child.primary.empty()) {
		std::sort(child.primary.begin(), child.primary.end());
		child.index(slot_);

		const int next = depth + 1;
		const std::uint64_t work = std::uint64_t{child.occ.size()} * child.primary.size();
		if (params_.threads > 1 && next <= kMaxSplitDepth && work >= kSplitWork) {
			std::shared_ptr<const Level> shared = child.materialize();
			std::vector<Task> tasks;
			tasks.reserve(child.primary.size());
			for (std::uint32_t c = 0; c < child.primary.size(); ++c)
				tasks.push_back({shared, next, c, prefix_items_});
			scheduler_.push(std::move(tasks));
		} else {
			for (const Item x : child.secondary) st_[x].mark = next;
			for (std::size_t c = 0; c < child.primary.size(); ++c) expand(child, next, c);
			for (const Item x : child.secondary) st_[x].mark = depth > 0 ? depth : -1;
		}
	}
	prefix_items_.pop_back();
}

}  // namespace efim
