// run_ours_tokenized_arith.cpp — tokenized GPDA parser on arithmetic
// grammars, using flex-generated arith lexer.  Grammar variant is
// selected at compile time via GRAMMAR_FN and PARSER_NAME.

#include "tokenized.hpp"
#include "bench_common.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>

#ifndef GRAMMAR_FN
#error "GRAMMAR_FN must be defined"
#endif
#ifndef PARSER_NAME
#error "PARSER_NAME must be defined"
#endif

extern "C" {
    struct yy_buffer_state;
    typedef yy_buffer_state* YY_BUFFER_STATE;
    // flex prefix "arith_" combined with flex's own yy_ prefix produces
    // double-underscore for the YY_* helpers.
    YY_BUFFER_STATE arith__scan_bytes(const char*, int);
    void arith__delete_buffer(YY_BUFFER_STATE);
    int arith_lex(void);
    extern char* arith_text;
    extern int arith_leng;
}

#include "arith.tab.h"

namespace gpda_tok { void GRAMMAR_FN(Graph& g); }

static const char* token_name(int id) {
    switch (id) {
        case NUMBER: return "NUMBER";
        case PLUS:   return "PLUS";
        case MINUS:  return "MINUS";
        case STAR:   return "STAR";
        case SLASH:  return "SLASH";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        default:     return "UNKNOWN";
    }
}

static std::vector<gpda_tok::Token> lex_all(const std::string& text) {
    std::vector<gpda_tok::Token> toks;
    toks.reserve(text.size() / 2);
    YY_BUFFER_STATE b = arith__scan_bytes(text.data(),
                                           static_cast<int>(text.size()));
    while (int id = arith_lex()) {
        toks.push_back({token_name(id),
                        std::string(arith_text, arith_leng), 0, 0});
    }
    arith__delete_buffer(b);
    return toks;
}

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

    gpda_tok::Parser parser;
    gpda_tok::GRAMMAR_FN(parser.graph);

    // Warmup
    for (int i = 0; i < 3; ++i) {
        auto tokens = lex_all(text);
        auto tree = parser.parse(tokens);
        sink = tree.get();
    }

    std::vector<double> times;
    times.reserve(iters);
    using clk = std::chrono::steady_clock;
    for (int i = 0; i < iters; ++i) {
        auto t0 = clk::now();
        auto tokens = lex_all(text);
        auto tree = parser.parse(tokens);
        auto t1 = clk::now();
        sink = tree.get();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        times.push_back(ms);
    }

    bench::emit_row(PARSER_NAME, input_label, text.size(),
                    iters, bench::compute(times));
    return 0;
}
