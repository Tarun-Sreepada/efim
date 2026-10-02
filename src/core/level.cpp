#include <efim/core/level.hpp>

#include <algorithm>
#include <numeric>

namespace efim {

void Level::clear() {
	items.clear();
	utils.clear();
	txs.clear();
	primary.clear();
	secondary.clear();
	occ_begin.clear();
	occ.clear();
}

void Level::add(const Item* it, const Utility* ut, std::uint32_t n, Utility prefix, bool view) {
	if (!txs.empty()) {
		Transaction& last = txs.back();
		if (last.len == n && std::equal(it, it + n, items_of(last))) {
			if (last.own < 0) {
				last.own = static_cast<std::int64_t>(items.size());
				items.insert(items.end(), last.items, last.items + n);
				utils.insert(utils.end(), last.utils, last.utils + n);
			}
			Utility* dst = utils.data() + last.own;
			for (std::uint32_t i = 0; i < n; ++i) dst[i] += ut[i];
			last.prefix += prefix;
			return;
		}
	}
	if (view) {
		txs.push_back({it, ut, n, prefix, -1});
	} else {
		txs.push_back({nullptr, nullptr, n, prefix, static_cast<std::int64_t>(items.size())});
		items.insert(items.end(), it, it + n);
		utils.insert(utils.end(), ut, ut + n);
	}
}

void Level::finalize() {
	for (auto& t : txs) {
		if (t.own < 0) continue;
		t.items = items.data() + t.own;
		t.utils = utils.data() + t.own;
	}
}

void Level::index(std::vector<std::int32_t>& slot) {
	for (std::size_t k = 0; k < primary.size(); ++k) slot[primary[k]] = static_cast<std::int32_t>(k);
	occ_begin.assign(primary.size() + 1, 0);
	for (const auto& t : txs)
		for (std::uint32_t i = 0; i < t.len; ++i)
			if (const std::int32_t s = slot[t.items[i]]; s >= 0) ++occ_begin[s + 1];
	std::partial_sum(occ_begin.begin(), occ_begin.end(), occ_begin.begin());
	occ.resize(occ_begin.back());
	cursor.assign(occ_begin.begin(), occ_begin.end() - 1);
	for (std::uint32_t tid = 0; tid < txs.size(); ++tid) {
		const auto& t = txs[tid];
		for (std::uint32_t i = 0; i < t.len; ++i)
			if (const std::int32_t s = slot[t.items[i]]; s >= 0) occ[cursor[s]++] = {tid, i};
	}
	for (const Item x : primary) slot[x] = -1;
}

std::shared_ptr<Level> Level::materialize() const {
	auto copy = std::make_shared<Level>();
	std::size_t total = 0;
	for (const auto& t : txs) total += t.len;
	copy->items.reserve(total);
	copy->utils.reserve(total);
	copy->txs.reserve(txs.size());
	for (const auto& t : txs) {
		copy->txs.push_back({nullptr, nullptr, t.len, t.prefix, static_cast<std::int64_t>(copy->items.size())});
		copy->items.insert(copy->items.end(), t.items, t.items + t.len);
		copy->utils.insert(copy->utils.end(), t.utils, t.utils + t.len);
	}
	copy->finalize();
	copy->primary = primary;
	copy->secondary = secondary;
	copy->occ_begin = occ_begin;
	copy->occ = occ;
	return copy;
}

}  // namespace efim
