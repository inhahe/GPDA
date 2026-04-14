// run_ours_scannerless_arith.cpp — benchmark driver for the scannerless
// GPDA parser on an arithmetic grammar.  The specific grammar variant
// (unambiguous vs ambiguous) is selected at compile time via the
// -D GRAMMAR_FN=<symbol> and -D PARSER_NAME=<"string"> defines.
#include "scannerless.hpp"
#include "bench_common.hpp"

#include <chrono>

#ifndef GRAMMAR_FN
#error "GRAMMAR_FN must be defined (e.g. build_arith_graph)"
#endif
#ifndef PARSER_NAME
#error "PARSER_NAME must be defined (e.g. \"ours_scannerless_arith\")"
#endif

namespace gpda { void GRAMMAR_FN(Graph& g); }

static volatile const void* sink = nullptr;

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <input> <input-label> <iters>\n";
        return 1;
    }
    const char* input_path  = argv[1];
    const char* input_label = argv[2];
    int iters = std::stoi(argv[3]);

    std::string text = bench::read_file(input_path);

    gpda::Parser parser;
    gpda::GRAMMAR_FN(parser.graph);

    // Warmup
    for (int i = 0; i < 3; ++i) {
        auto tree = parser.parse(text);
        sink = tree.get();
    }

    std::vector<double> times;
    times.reserve(iters);
    using clk = std::chrono::steady_clock;
    for (int i = 0; i < iters; ++i) {
        auto t0 = clk::now();
        auto tree = parser.parse(text);
        auto t1 = clk::now();
        sink = tree.get();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        times.push_back(ms);
    }

    bench::emit_row(PARSER_NAME, input_label, text.size(),
                    iters, bench::compute(times));
    return 0;
}
