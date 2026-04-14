// test_scannerless.cpp — sanity tests for the C++ scannerless parser.
//
// Grammars are hand-built here to avoid depending on a C++ bootstrap.
// For the benchmark phase we'll have Python emit grammar-construction
// source; this file just verifies the parser mechanics.

#include "scannerless.hpp"

#include <iostream>
#include <string>

using namespace gpda;

// ---- helpers -------------------------------------------------------------

static int passed = 0;
static int failed = 0;

static void check(const std::string& name, bool cond) {
    if (cond) { std::cout << "  PASS: " << name << "\n"; ++passed; }
    else      { std::cout << "  FAIL: " << name << "\n"; ++failed; }
}

// Link from → to
static void link(Graph& g, std::uint32_t from, std::uint32_t to) {
    g.nodes[from].links.push_back(to);
}

// Build a plain match-char node
static std::uint32_t make_char(Graph& g, char c) {
    std::uint32_t id = g.add_node(NodeType::MatchChar);
    g.nodes[id].char_val = c;
    return id;
}

// Build a match-class node from a single range
static std::uint32_t make_range(Graph& g, char lo, char hi,
                                bool negated = false) {
    std::uint32_t id = g.add_node(NodeType::MatchClass);
    g.nodes[id].cclass.ranges.push_back({lo, hi});
    g.nodes[id].cclass.negated = negated;
    return id;
}

// ---- Grammar builders ----------------------------------------------------

// g = 'hello'
static Graph build_literal() {
    Graph g;
    g.start_rule = "g";
    auto [gs, ge] = g.add_rule("g");
    std::uint32_t prev = gs;
    for (char c : std::string("hello")) {
        std::uint32_t n = make_char(g, c);
        link(g, prev, n);
        prev = n;
    }
    link(g, prev, ge);
    return g;
}

// digits = [0-9]+
static Graph build_digits_plus() {
    Graph g;
    g.start_rule = "digits";
    auto [rs, re] = g.add_rule("digits");
    std::uint32_t d = make_range(g, '0', '9');
    std::uint32_t loop = g.add_node(NodeType::Split);
    std::uint32_t exit_ = g.add_node(NodeType::Split);

    link(g, rs, d);         // start -> first d
    link(g, d, loop);       // d -> loop (one-or-more: must match once first)
    link(g, loop, d);       // loop -> d    (greedy: try more)
    link(g, loop, exit_);   // loop -> exit
    link(g, exit_, re);     // exit -> end
    return g;
}

// expr = term '+' term
// term = [0-9]+
static Graph build_addition() {
    Graph g;
    g.start_rule = "expr";

    // term rule
    auto [ts, te] = g.add_rule("term");
    std::uint32_t d = make_range(g, '0', '9');
    std::uint32_t loop = g.add_node(NodeType::Split);
    std::uint32_t exit_ = g.add_node(NodeType::Split);
    link(g, ts, d);
    link(g, d, loop);
    link(g, loop, d);
    link(g, loop, exit_);
    link(g, exit_, te);

    // expr rule
    auto [es, ee] = g.add_rule("expr");
    std::uint32_t t1 = g.add_node(NodeType::RuleRef);
    g.nodes[t1].name = "term";
    std::uint32_t plus = make_char(g, '+');
    std::uint32_t t2 = g.add_node(NodeType::RuleRef);
    g.nodes[t2].name = "term";

    link(g, es, t1);
    link(g, t1, plus);
    link(g, plus, t2);
    link(g, t2, ee);
    return g;
}

// g = 'a' | 'b'
static Graph build_alternatives() {
    Graph g;
    g.start_rule = "g";
    auto [gs, ge] = g.add_rule("g");
    std::uint32_t a = make_char(g, 'a');
    std::uint32_t b = make_char(g, 'b');
    link(g, gs, a);
    link(g, gs, b);
    link(g, a, ge);
    link(g, b, ge);
    return g;
}

// g = . . .       (three arbitrary characters)
static Graph build_any_triple() {
    Graph g;
    g.start_rule = "g";
    auto [gs, ge] = g.add_rule("g");
    std::uint32_t a = g.add_node(NodeType::MatchAny);
    std::uint32_t b = g.add_node(NodeType::MatchAny);
    std::uint32_t c = g.add_node(NodeType::MatchAny);
    link(g, gs, a);
    link(g, a, b);
    link(g, b, c);
    link(g, c, ge);
    return g;
}

// Recursive grammar (from the blog post):
//   s = a '#'
//   a = 'a' a 'b' | 'c'
static Graph build_recursive() {
    Graph g;
    g.start_rule = "s";

    auto [as, ae] = g.add_rule("a");
    // Alt 1: 'a' a 'b'
    std::uint32_t a_lit = make_char(g, 'a');
    std::uint32_t a_rec = g.add_node(NodeType::RuleRef);
    g.nodes[a_rec].name = "a";
    std::uint32_t b_lit = make_char(g, 'b');
    link(g, as, a_lit);
    link(g, a_lit, a_rec);
    link(g, a_rec, b_lit);
    link(g, b_lit, ae);
    // Alt 2: 'c'
    std::uint32_t c_lit = make_char(g, 'c');
    link(g, as, c_lit);
    link(g, c_lit, ae);

    auto [ss, se] = g.add_rule("s");
    std::uint32_t a_ref = g.add_node(NodeType::RuleRef);
    g.nodes[a_ref].name = "a";
    std::uint32_t hash = make_char(g, '#');
    link(g, ss, a_ref);
    link(g, a_ref, hash);
    link(g, hash, se);
    return g;
}

// Hand-built graph for `/[\u00a0-\u00ff]+/u` after Unicode expansion —
// the same shape the Python emitter produces for this regex.  Codepoint
// range [0xA0, 0xFF] decomposes into two UTF-8 byte alternatives:
//   (0xC2)(0xA0-0xBF)   for codepoints 0xA0..0xBF
//   (0xC3)(0x80-0xBF)   for codepoints 0xC0..0xFF
// The `+` quantifier wraps the whole alternation.
static Graph build_utf8_class() {
    Graph g;
    g.start_rule = "s";
    auto [ss, se] = g.add_rule("s");

    // Alt 1: 0xC2 [0xA0-0xBF]
    auto c2 = make_char(g, static_cast<char>(0xC2));
    auto c2_tail = make_range(g, static_cast<char>(0xA0), static_cast<char>(0xBF));
    link(g, c2, c2_tail);

    // Alt 2: 0xC3 [0x80-0xBF]
    auto c3 = make_char(g, static_cast<char>(0xC3));
    auto c3_tail = make_range(g, static_cast<char>(0x80), static_cast<char>(0xBF));
    link(g, c3, c3_tail);

    // Alt-split: entry -> each alternative's first node
    auto alt_split = g.add_node(NodeType::Split);
    link(g, alt_split, c2);
    link(g, alt_split, c3);

    // Alt-join: both alternatives' tails -> join (single next node)
    auto alt_join = g.add_node(NodeType::Split);
    link(g, c2_tail, alt_join);
    link(g, c3_tail, alt_join);

    // `+` quantifier: alt_join -> loop_split -> [alt_split (loop), exit]
    auto loop_split = g.add_node(NodeType::Split);
    auto exit_split = g.add_node(NodeType::Split);
    link(g, alt_join, loop_split);
    link(g, loop_split, alt_split);   // loop
    link(g, loop_split, exit_split);  // done

    // Rule body: s -> alt_split ... exit_split -> s_end
    link(g, ss, alt_split);
    link(g, exit_split, se);
    return g;
}

// EBNF `{"a"} - "aa"` — zero-or-more 'a' with the span "aa" excluded.
// Compiles `A - B` as an anonymous stripped rule `_sub = A <SubCheckNot(B)>`
// so SubCheckNot can grab A's start position from stack[-1].start_pos.
static Graph build_ebnf_subtract() {
    Graph g;
    g.start_rule = "s";

    // A = {"a"}: entry SPLIT → match 'a' → loop back; or → exit
    auto [rs_sub, re_sub] = g.add_rule("_sub");
    g.stripped_names.insert("_sub");
    std::uint32_t a_char = make_char(g, 'a');
    std::uint32_t entry = g.add_node(NodeType::Split);
    std::uint32_t star_exit = g.add_node(NodeType::Split);
    link(g, entry, a_char);       // greedy: try 'a' first
    link(g, entry, star_exit);    // or exit
    link(g, a_char, entry);       // loop back after matching

    // B = "aa": predicate wraps a,a sequence in its own start/end so
    // evaluate_predicate_bounded can walk it.
    std::uint32_t b_rs = g.add_node(NodeType::RuleStart);
    std::uint32_t b_re = g.add_node(NodeType::RuleEnd);
    g.nodes[b_rs].name = "_sub_pred";
    g.nodes[b_re].name = "_sub_pred";
    std::uint32_t aa1 = make_char(g, 'a');
    std::uint32_t aa2 = make_char(g, 'a');
    link(g, b_rs, aa1);
    link(g, aa1, aa2);
    link(g, aa2, b_re);

    // SubCheckNot node with pred_start = B's graph
    std::uint32_t check = g.add_node(NodeType::SubCheckNot);
    g.nodes[check].pred_start = b_rs;

    // Wire the anonymous _sub rule body: rs -> entry -> ... -> check -> re
    link(g, rs_sub, entry);
    link(g, star_exit, check);
    link(g, check, re_sub);

    // Outer rule s = _sub (one ref)
    auto [ss, se] = g.add_rule("s");
    std::uint32_t sub_ref = g.add_node(NodeType::RuleRef);
    g.nodes[sub_ref].name = "_sub";
    link(g, ss, sub_ref);
    link(g, sub_ref, se);
    return g;
}

// ---- Tests ---------------------------------------------------------------

int main() {
    // --- Test 1: literal string ---
    std::cout << "--- Test 1: literal string 'hello' ---\n";
    {
        Parser p; p.graph = build_literal();
        auto tree = p.parse("hello");
        check("matches 'hello'", tree && tree->name == "g" && tree->text == "hello");
        bool threw = false;
        try { p.parse("hell"); } catch (...) { threw = true; }
        check("rejects 'hell'", threw);
        threw = false;
        try { p.parse("helloo"); } catch (...) { threw = true; }
        check("rejects 'helloo'", threw);
    }

    // --- Test 2: [0-9]+ ---
    std::cout << "\n--- Test 2: [0-9]+ ---\n";
    {
        Parser p; p.graph = build_digits_plus();
        auto t1 = p.parse("1");
        check("matches '1'", t1 && t1->text == "1");
        auto t2 = p.parse("42");
        check("matches '42'", t2 && t2->text == "42");
        auto t3 = p.parse("123456");
        check("matches '123456'", t3 && t3->text == "123456");
        bool threw = false;
        try { p.parse(""); } catch (...) { threw = true; }
        check("rejects empty", threw);
        threw = false;
        try { p.parse("abc"); } catch (...) { threw = true; }
        check("rejects 'abc'", threw);
    }

    // --- Test 3: expr = term '+' term with nested rules ---
    std::cout << "\n--- Test 3: expr = term '+' term ---\n";
    {
        Parser p; p.graph = build_addition();
        auto tree = p.parse("12+34");
        check("matches '12+34'", tree && tree->text == "12+34");
        check("root is 'expr'", tree->name == "expr");
        check("2 term children", tree->children.size() == 2);
        check("first term is '12'",
              tree->children[0]->name == "term" &&
              tree->children[0]->text == "12");
        check("second term is '34'",
              tree->children[1]->name == "term" &&
              tree->children[1]->text == "34");
        std::cout << tree->pretty() << "\n";
    }

    // --- Test 4: alternatives ---
    std::cout << "\n--- Test 4: 'a' | 'b' ---\n";
    {
        Parser p; p.graph = build_alternatives();
        auto ta = p.parse("a"); check("matches 'a'", ta && ta->text == "a");
        auto tb = p.parse("b"); check("matches 'b'", tb && tb->text == "b");
        bool threw = false;
        try { p.parse("c"); } catch (...) { threw = true; }
        check("rejects 'c'", threw);
    }

    // --- Test 5: any character (.) ---
    std::cout << "\n--- Test 5: . . . ---\n";
    {
        Parser p; p.graph = build_any_triple();
        auto t = p.parse("xyz");
        check("matches 'xyz'", t && t->text == "xyz");
        auto t2 = p.parse("!@#");
        check("matches '!@#'", t2 && t2->text == "!@#");
        bool threw = false;
        try { p.parse("ab"); } catch (...) { threw = true; }
        check("rejects 2-char input", threw);
    }

    // --- Test 6: recursive grammar (blog post example) ---
    std::cout << "\n--- Test 6: S = A '#', A = 'a' A 'b' | 'c' ---\n";
    {
        Parser p; p.graph = build_recursive();
        auto t1 = p.parse("c#");
        check("matches 'c#'", t1 && t1->name == "s");
        auto t2 = p.parse("acb#");
        check("matches 'acb#'", t2 && t2->name == "s");
        auto t3 = p.parse("aacbb#");
        check("matches 'aacbb#'", t3 && t3->name == "s");
        auto t4 = p.parse("aaacbbb#");
        check("matches 'aaacbbb#'", t4 && t4->name == "s");
        bool threw = false;
        try { p.parse("ab#"); } catch (...) { threw = true; }
        check("rejects 'ab#'", threw);
    }

    // --- UTF-8 Unicode char-class `[\u00a0-\u00ff]+` ---
    std::cout << "\n--- Unicode class [\\u00a0-\\u00ff]+ under /u ---\n";
    {
        Parser p; p.graph = build_utf8_class();
        // 'é' = U+00E9 = UTF-8 bytes 0xC3 0xA9 (in range 0xA0..0xFF)
        // 'À' = U+00C0 = UTF-8 bytes 0xC3 0x80
        // 'à' = U+00E0 = UTF-8 bytes 0xC3 0xA0
        std::string eacute = "\xC3\xA9";
        auto t = p.parse(eacute);
        check("matches single 2-byte codepoint (é)",
              t && t->text == eacute);

        // Multi-codepoint: 'éàü' = 6 UTF-8 bytes
        std::string multi = "\xC3\xA9\xC3\xA0\xC3\xBC";
        auto t2 = p.parse(multi);
        check("matches 3 codepoints (éàü, 6 bytes)",
              t2 && t2->text == multi);

        // Boundary: 0xC2 0xA0 is U+00A0, the lo end
        std::string a0 = "\xC2\xA0";
        check("matches lo-boundary codepoint U+00A0",
              p.parse(a0)->text == a0);

        // Out of range: 0x41 = 'A', single byte, not in [0xA0-0xFF]
        bool threw = false;
        try { p.parse("A"); } catch (...) { threw = true; }
        check("rejects ASCII 'A' (not in unicode range)", threw);

        // Out of range on the other side: 0xC2 0x80 = U+0080 (below 0xA0)
        std::string below = "\xC2\x80";
        threw = false;
        try { p.parse(below); } catch (...) { threw = true; }
        check("rejects U+0080 (below lo of range)", threw);
    }

    // --- Test 7: EBNF `A - B` subtraction via SubCheckNot ---
    std::cout << "\n--- Test 7: {\"a\"} - \"aa\" ---\n";
    {
        Parser p; p.graph = build_ebnf_subtract();
        auto t1 = p.parse("a");
        check("matches 'a'", t1 && t1->name == "s");
        auto t2 = p.parse("aaa");
        check("matches 'aaa'", t2 && t2->name == "s");
        auto t3 = p.parse("");
        check("matches '' (empty)", t3 && t3->name == "s");
        bool threw = false;
        try { p.parse("aa"); } catch (...) { threw = true; }
        check("rejects 'aa' (subtract match)", threw);
    }

    // --- Summary ---
    std::cout << "\n========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    if (failed) { std::cout << "SOME TESTS FAILED\n"; return 1; }
    std::cout << "ALL TESTS PASSED\n";
    return 0;
}
