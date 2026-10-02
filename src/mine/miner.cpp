#include <efim/mine/miner.hpp>

#include <algorithm>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>

#include <efim/io/dataset.hpp>
#include <efim/io/pattern_writer.hpp>
#include <efim/mine/scheduler.hpp>
#include <efim/util/sys.hpp>
#include <efim/mine/worker.hpp>

namespace efim {

MineResult mine(const MineOptions& opt) {
	using clock = std::chrono::steady_clock;
	const auto start = clock::now();

	MineResult result;
	const int max_threads = hardware_threads();
	const int threads = opt.threads <= 0 ? max_threads : std::clamp(opt.threads, 1, max_threads);
	if (opt.threads > max_threads)
		spdlog::warn("threads {} capped to {} (allowed range 1-{})", opt.threads, threads, max_threads);
	result.threads = threads;

	PatternWriter writer(opt.output);

	spdlog::info("loading {}", opt.input);
	Dataset data = Dataset::load(opt.input, opt.sep, opt.min_util);
	result.load_seconds = std::chrono::duration<double>(clock::now() - start).count();
	spdlog::info("{} transactions, {} promising items, {} root extensions ({:.2f}s)", data.num_transactions,
				 data.num_items, data.root->primary.size(), result.load_seconds);

	Scheduler scheduler;
	{
		std::vector<Task> tasks;
		for (std::uint32_t k = 0; k < data.root->primary.size(); ++k) tasks.push_back({data.root, 0, k, {}});
		scheduler.push(std::move(tasks));
	}
	data.root.reset();  // the tasks keep the root alive until they finish

	const SearchParams params{opt.min_util, threads};
	std::vector<std::unique_ptr<Worker>> workers;
	for (int i = 0; i < threads; ++i) workers.push_back(std::make_unique<Worker>(data, params, scheduler, writer));

	spdlog::info("mining with {} thread(s)", threads);
	if (threads == 1) {
		workers[0]->run();
	} else {
		std::vector<std::thread> pool;
		for (auto& w : workers) pool.emplace_back(&Worker::run, w.get());
		for (auto& t : pool) t.join();
	}
	for (auto& w : workers) {
		w->flush();
		result.patterns += w->pattern_count();
	}
	writer.close();

	result.seconds = std::chrono::duration<double>(clock::now() - start).count();
	result.peak_rss = peak_rss_bytes();
	return result;
}

}  // namespace efim
