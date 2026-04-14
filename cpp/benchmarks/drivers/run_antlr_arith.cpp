// run_antlr_arith.cpp — ANTLR4 benchmark driver for arithmetic.
#include "bench_common.hpp"

#include "antlr4-runtime.h"
#include "ArithLexer.h"
#include "ArithParser.h"

#include <chrono>

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

    auto do_parse = [&]() {
        antlr4::ANTLRInputStream input(text);
        ArithLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        ArithParser parser(&tokens);
        parser.removeErrorListeners();
        lexer.removeErrorListeners();
        auto* tree = parser.main();
        return tree;
    };

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

    bench::emit_row("antlr4_arith", input_label, text.size(),
                    iters, bench::compute(times));
    return 0;
}
