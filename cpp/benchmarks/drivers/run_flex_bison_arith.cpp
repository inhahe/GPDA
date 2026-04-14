// run_flex_bison_arith.cpp — flex+bison benchmark driver for arithmetic.
#include "bench_common.hpp"

#include <chrono>
#include <cstdio>

extern "C" {
    int arith_parse(void);
    struct yy_buffer_state;
    typedef yy_buffer_state* YY_BUFFER_STATE;
    // flex prefix "arith_" combined with its own yy_ prefix produces
    // arith__scan_bytes (double underscore).
    YY_BUFFER_STATE arith__scan_bytes(const char*, int);
    void arith__delete_buffer(YY_BUFFER_STATE);
}

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

    // Warmup
    for (int i = 0; i < 3; ++i) {
        YY_BUFFER_STATE b = arith__scan_bytes(text.data(),
                                               static_cast<int>(text.size()));
        int rc = arith_parse();
        arith__delete_buffer(b);
        if (rc != 0) {
            std::cerr << "flex+bison arith parse failed on "
                      << input_path << "\n";
            return 3;
        }
    }

    std::vector<double> times;
    times.reserve(iters);
    using clk = std::chrono::steady_clock;
    for (int i = 0; i < iters; ++i) {
        auto t0 = clk::now();
        YY_BUFFER_STATE b = arith__scan_bytes(text.data(),
                                               static_cast<int>(text.size()));
        int rc = arith_parse();
        arith__delete_buffer(b);
        auto t1 = clk::now();
        if (rc != 0) {
            std::cerr << "flex+bison arith parse failed\n";
            return 3;
        }
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        times.push_back(ms);
    }

    bench::emit_row("flex_bison_arith", input_label, text.size(),
                    iters, bench::compute(times));
    return 0;
}
