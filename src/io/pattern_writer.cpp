#include <efim/io/pattern_writer.hpp>

#include <stdexcept>

namespace efim {

PatternWriter::PatternWriter(const std::string& path) : file_(std::fopen(path.c_str(), "wb")) {
	if (!file_) throw std::runtime_error("cannot open output file: " + path);
}

PatternWriter::~PatternWriter() {
	if (file_) std::fclose(file_);
}

void PatternWriter::write(const std::string& chunk) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (std::fwrite(chunk.data(), 1, chunk.size(), file_) != chunk.size())
		throw std::runtime_error("write to output file failed");
}

void PatternWriter::close() {
	if (file_ && std::fclose(file_) != 0) {
		file_ = nullptr;
		throw std::runtime_error("closing output file failed");
	}
	file_ = nullptr;
}

}  // namespace efim
