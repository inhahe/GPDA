// bench_common.hpp — shared timing + I/O helpers for all benchmark drivers.
#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace bench {

inline std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "Cannot open " << path << "\n";
        std::exit(2);
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

struct Stats {
    double min_ms;
    double median_ms;
    double max_ms;
};

inline Stats compute(std::vector<double> times) {
    std::sort(times.begin(), times.end());
    Stats s;
    s.min_ms    = times.front();
    s.median_ms = times[times.size() / 2];
    s.max_ms    = times.back();
    return s;
}

// Emit one row of CSV to stdout.
// Columns: parser, input, size_bytes, iters, min_ms, median_ms, max_ms
inline void emit_row(const std::string& parser_name,
                     const std::string& input_name,
                     std::size_t size_bytes,
                     int iters,
                     const Stats& s) {
    std::cout << parser_name << ","
              << input_name << ","
              << size_bytes << ","
              << iters << ","
              << s.min_ms << ","
              << s.median_ms << ","
              << s.max_ms << "\n";
}

}  // namespace bench
