#pragma once

#include <cstdio>
#include <mutex>
#include <string>

namespace efim {

// Thread-safe sink for formatted pattern lines. Workers format into their own buffer
// and hand it over in large chunks.
class PatternWriter {
public:
	explicit PatternWriter(const std::string& path);
	~PatternWriter();
	PatternWriter(const PatternWriter&) = delete;
	PatternWriter& operator=(const PatternWriter&) = delete;

	void write(const std::string& chunk);
	void close();

private:
	std::FILE* file_ = nullptr;
	std::mutex mutex_;
};

}  // namespace efim
