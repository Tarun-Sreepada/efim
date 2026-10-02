#pragma once

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include <efim/core/level.hpp>
#include <efim/core/types.hpp>

namespace efim {

// Extend `prefix` with level->primary[slot], where `level` is the database at `depth`.
struct Task {
	std::shared_ptr<const Level> level;
	int depth;
	std::uint32_t slot;
	std::vector<Item> prefix;
};

// LIFO task stack shared by the workers; pop() returns false once all work is done.
class Scheduler {
public:
	void push(std::vector<Task>&& tasks) {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			outstanding_ += tasks.size();
			// Reversed so the lowest slot is popped first.
			for (auto it = tasks.rbegin(); it != tasks.rend(); ++it) stack_.push_back(std::move(*it));
		}
		cv_.notify_all();
	}

	bool pop(Task& out) {
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock, [&] { return !stack_.empty() || outstanding_ == 0; });
		if (stack_.empty()) return false;
		out = std::move(stack_.back());
		stack_.pop_back();
		return true;
	}

	void done() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (--outstanding_ == 0) cv_.notify_all();
	}

private:
	std::mutex mutex_;
	std::condition_variable cv_;
	std::vector<Task> stack_;
	std::size_t outstanding_ = 0;  // queued + running tasks
};

}  // namespace efim
