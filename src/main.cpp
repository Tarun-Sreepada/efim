// Usage: efim <input> <output> <minUtil> [--sep tab|space|<literal>] [--threads N] [-v]
//
// Input format (one transaction per line, items/utilities split by the separator):
//     a c d:8:5 1 2

#include <iostream>
#include <string>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include <efim/mine/miner.hpp>

namespace {

// "tab" and "space" are names for separators that are awkward to type in a shell.
std::string parse_sep(const std::string& arg) {
	if (arg == "tab") return "\t";
	if (arg == "space") return " ";
	return arg;
}

}  // namespace

int main(int argc, char** argv) {
	CLI::App app{"EFIM high-utility itemset miner"};
	efim::MineOptions opt;
	std::string sep = "tab";
	bool verbose = false;

	app.add_option("input", opt.input, "Input file")->required()->check(CLI::ExistingFile);
	app.add_option("output", opt.output, "Output file")->required();
	app.add_option("minutil", opt.min_util, "Minimum utility threshold")->required()->check(CLI::NonNegativeNumber);
	app.add_option("-s,--sep", sep, "Separator: tab, space, or a literal like ; or ,")->capture_default_str();
	app.add_option("-t,--threads", opt.threads, "Worker threads (0 = all hardware threads)")
		->capture_default_str();
	app.add_flag("-v,--verbose", verbose, "Debug logging");
	CLI11_PARSE(app, argc, argv);

	spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
	if (verbose) spdlog::set_level(spdlog::level::debug);
	opt.sep = parse_sep(sep);

	try {
		const efim::MineResult r = efim::mine(opt);
		// Plain, stable lines on stdout for scripts; progress goes to stderr via spdlog.
		std::cout << "Total number of High Utility Patterns: " << r.patterns << '\n'
				  << "Peak Memory in RSS: " << r.peak_rss << '\n'
				  << "Total ExecutionTime in seconds: " << r.seconds << '\n';
	} catch (const std::exception& e) {
		spdlog::error("{}", e.what());
		return 1;
	}
}
