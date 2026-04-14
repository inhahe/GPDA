// run_antlr.cpp — benchmark driver for the ANTLR4-generated JSON parser.
#include "bench_common.hpp"

#include "antlr4-runtime.h"
#include "JSONLexer.h"
#include "JSONParser.h"

#include <chrono>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.json> <input-label> <iters>\n";
        return 1;
    }
    const char* input_path  = argv[1];
    const char* input_label = argv[2];
    int iters = std::stoi(argv[3]);

    std::string text = bench::read_file(input_path);

    auto do_parse = [&]() {
        antlr4::ANTLRInputStream input(text);
        JSONLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        JSONParser parser(&tokens);
        // Turn off error listeners for speed (we're measuring pure parse time).
        parser.removeErrorListeners();
        lexer.removeErrorListeners();
        auto* tree = parser.json();
        return tree;
    };

    // Warmup
    for (int i = 0; i < 3; ++i) (void)do_parse();

    std::vector<double> times;
    times.reserve(iters);
    using clk = std::chrono::steady_clock;
    for (int i = 0; i < iters; ++i) {
        auto t0 = clk::now();
        auto* tree = do_parse();
        auto t1 = clk::now();
        (void)tree;
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        times.push_back(ms);
    }

    bench::emit_row("antlr4", input_label, text.size(),
                    iters, bench::compute(times));
    return 0;
}
