// run_flex_bison.cpp — benchmark driver for the flex+bison JSON parser.
#include "bench_common.hpp"

#include <chrono>
#include <cstdio>

extern "C" {
    int yyparse(void);
    // flex exposes YY_BUFFER_STATE and helpers for in-memory parsing
    struct yy_buffer_state;
    typedef yy_buffer_state* YY_BUFFER_STATE;
    YY_BUFFER_STATE yy_scan_bytes(const char* bytes, int len);
    void yy_delete_buffer(YY_BUFFER_STATE b);
}

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

    // Warmup
    for (int i = 0; i < 3; ++i) {
        YY_BUFFER_STATE b = yy_scan_bytes(text.data(),
                                           static_cast<int>(text.size()));
        int rc = yyparse();
        yy_delete_buffer(b);
        if (rc != 0) {
            std::cerr << "flex+bison parse failed on " << input_path << "\n";
            return 3;
        }
    }

    std::vector<double> times;
    times.reserve(iters);
    using clk = std::chrono::steady_clock;
    for (int i = 0; i < iters; ++i) {
        auto t0 = clk::now();
        YY_BUFFER_STATE b = yy_scan_bytes(text.data(),
                                           static_cast<int>(text.size()));
        int rc = yyparse();
        yy_delete_buffer(b);
        auto t1 = clk::now();
        if (rc != 0) {
            std::cerr << "flex+bison parse failed\n";
            return 3;
        }
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        times.push_back(ms);
    }

    bench::emit_row("flex_bison", input_label, text.size(),
                    iters, bench::compute(times));
    return 0;
}
