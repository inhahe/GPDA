// run_ours_tokenized.cpp — tokenized GPDA parser with flex as the lexer.
//
// The flex lexer generated from json.l returns integer token IDs defined
// in json.tab.h (LBRACE, RBRACE, ...).  This driver translates those to
// gpda_tok::Token{type, value} form and hands them to our parser.

#include "tokenized.hpp"
#include "bench_common.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>

extern "C" {
    // From the flex-generated lexer:
    struct yy_buffer_state;
    typedef yy_buffer_state* YY_BUFFER_STATE;
    YY_BUFFER_STATE yy_scan_bytes(const char*, int);
    void yy_delete_buffer(YY_BUFFER_STATE);
    int yylex(void);
    extern char* yytext;
    extern int yyleng;
}

// Token IDs from json.tab.h
#include "json.tab.h"

namespace gpda_tok { void build_json_graph(Graph& g); }

static const char* token_name(int id) {
    switch (id) {
        case LBRACE:   return "LBRACE";
        case RBRACE:   return "RBRACE";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case COMMA:    return "COMMA";
        case COLON:    return "COLON";
        case TRUE_KW:  return "TRUE_KW";
        case FALSE_KW: return "FALSE_KW";
        case NULL_KW:  return "NULL_KW";
        case STRING:   return "STRING";
        case NUMBER:   return "NUMBER";
        default:       return "UNKNOWN";
    }
}

// Tokenize one pass, filling out a Token vector.
static std::vector<gpda_tok::Token> lex_all(const std::string& text) {
    std::vector<gpda_tok::Token> toks;
    toks.reserve(text.size() / 4);  // rough estimate
    YY_BUFFER_STATE b = yy_scan_bytes(text.data(),
                                       static_cast<int>(text.size()));
    while (int id = yylex()) {
        toks.push_back({token_name(id),
                        std::string(yytext, yyleng), 0, 0});
    }
    yy_delete_buffer(b);
    return toks;
}

static volatile const void* sink = nullptr;

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

    gpda_tok::Parser parser;
    build_json_graph(parser.graph);

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

    bench::emit_row("ours_tokenized", input_label, text.size(),
                    iters, bench::compute(times));
    return 0;
}
