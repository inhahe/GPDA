// test_tokenized.cpp — sanity tests for the C++ tokenized parser.
//
// Tokens are constructed by hand; grammar graphs are built in code.
// No lexer is used here — the flex integration comes next session.

#include "tokenized.hpp"

#include <iostream>
#include <string>

using namespace gpda_tok;

// ---- helpers -------------------------------------------------------------

static int passed = 0;
static int failed = 0;

static void check(const std::string& name, bool cond) {
    if (cond) { std::cout << "  PASS: " << name << "\n"; ++passed; }
    else      { std::cout << "  FAIL: " << name << "\n"; ++failed; }
}

static void link(Graph& g, std::uint32_t from, std::uint32_t to) {
    g.nodes[from].links.push_back(to);
}

static std::uint32_t make_str(Graph& g, const std::string& s) {
    std::uint32_t id = g.add_node(NodeType::MatchStr);
    g.nodes[id].value = s;
    return id;
}

static std::uint32_t make_tok(Graph& g, const std::string& type_name) {
    std::uint32_t id = g.add_node(NodeType::MatchTok);
    g.nodes[id].value = type_name;
    return id;
}

static Token tok(const std::string& type, const std::string& value,
                 std::uint32_t line = 1, std::uint32_t col = 1) {
    return Token{type, value, line, col};
}

// ---- Grammars ------------------------------------------------------------

// g = NUMBER
static Graph build_single_token() {
    Graph g;
    g.start_rule = "g";
    auto [gs, ge] = g.add_rule("g");
    std::uint32_t num = make_tok(g, "NUMBER");
    link(g, gs, num);
    link(g, num, ge);
    return g;
}

// g = NUMBER '+' NUMBER       (literal '+' matches token value)
static Graph build_addition() {
    Graph g;
    g.start_rule = "g";
    auto [gs, ge] = g.add_rule("g");
    std::uint32_t n1 = make_tok(g, "NUMBER");
    std::uint32_t plus = make_str(g, "+");
    std::uint32_t n2 = make_tok(g, "NUMBER");
    link(g, gs, n1);
    link(g, n1, plus);
    link(g, plus, n2);
    link(g, n2, ge);
    return g;
}

// g = 'foo' | 'bar'
static Graph build_alternatives() {
    Graph g;
    g.start_rule = "g";
    auto [gs, ge] = g.add_rule("g");
    std::uint32_t foo = make_str(g, "foo");
    std::uint32_t bar = make_str(g, "bar");
    link(g, gs, foo);
    link(g, gs, bar);
    link(g, foo, ge);
    link(g, bar, ge);
    return g;
}

// Skip-type test:
//   _ws = <skip marker>
//   g = NUMBER '+' NUMBER
// with _ws in skip_types.  WS tokens between NUMBER/+ should be invisible.
static Graph build_skip_ws() {
    Graph g = build_addition();
    g.skip_types.insert("WS");
    return g;
}

// Skip + explicit reference:
//   g = LET WS IDENT
// where WS is in skip_types.  Because the grammar explicitly mentions WS,
// its tokens are required at that position even though they're "skippable".
static Graph build_let_with_explicit_ws() {
    Graph g;
    g.start_rule = "g";
    g.skip_types.insert("WS");
    auto [gs, ge] = g.add_rule("g");
    std::uint32_t let = make_tok(g, "LET");
    std::uint32_t ws  = make_tok(g, "WS");
    std::uint32_t id  = make_tok(g, "IDENT");
    link(g, gs, let);
    link(g, let, ws);
    link(g, ws, id);
    link(g, id, ge);
    return g;
}

// Nested rules:
//   expr = term '+' term
//   term = NUMBER
static Graph build_nested() {
    Graph g;
    g.start_rule = "expr";

    auto [ts, te] = g.add_rule("term");
    std::uint32_t num = make_tok(g, "NUMBER");
    link(g, ts, num);
    link(g, num, te);

    auto [es, ee] = g.add_rule("expr");
    std::uint32_t t1 = g.add_node(NodeType::RuleRef); g.nodes[t1].value = "term";
    std::uint32_t plus = make_str(g, "+");
    std::uint32_t t2 = g.add_node(NodeType::RuleRef); g.nodes[t2].value = "term";
    link(g, es, t1);
    link(g, t1, plus);
    link(g, plus, t2);
    link(g, t2, ee);
    return g;
}

// Kleene star:
//   items = NUMBER ('+' NUMBER)*
static Graph build_chained_plus() {
    Graph g;
    g.start_rule = "items";
    auto [s, e] = g.add_rule("items");

    std::uint32_t first = make_tok(g, "NUMBER");
    std::uint32_t entry = g.add_node(NodeType::Split);
    std::uint32_t plus  = make_str(g, "+");
    std::uint32_t num   = make_tok(g, "NUMBER");
    std::uint32_t exit_ = g.add_node(NodeType::Split);

    link(g, s, first);
    link(g, first, entry);
    link(g, entry, plus);   // greedy: try matching the loop body first
    link(g, entry, exit_);  // or exit
    link(g, plus, num);
    link(g, num, entry);    // loop back
    link(g, exit_, e);
    return g;
}

// EBNF `{"a"} - ("a","a")` — zero-or-more 'a' tokens, reject the 2-'a' span.
// Same pattern as the scannerless version: anonymous stripped rule
// `_sub = A <SubCheckNot(B)>`.
static Graph build_ebnf_subtract() {
    Graph g;
    g.start_rule = "s";

    auto [rs_sub, re_sub] = g.add_rule("_sub");
    g.stripped_names.insert("_sub");

    std::uint32_t a_tok = make_str(g, "a");
    std::uint32_t entry = g.add_node(NodeType::Split);
    std::uint32_t star_exit = g.add_node(NodeType::Split);
    link(g, entry, a_tok);
    link(g, entry, star_exit);
    link(g, a_tok, entry);

    // B = "a","a"
    std::uint32_t b_rs = g.add_node(NodeType::RuleStart);
    std::uint32_t b_re = g.add_node(NodeType::RuleEnd);
    g.nodes[b_rs].rule_name = "_sub_pred";
    g.nodes[b_re].rule_name = "_sub_pred";
    std::uint32_t a1 = make_str(g, "a");
    std::uint32_t a2 = make_str(g, "a");
    link(g, b_rs, a1);
    link(g, a1, a2);
    link(g, a2, b_re);

    std::uint32_t check = g.add_node(NodeType::SubCheckNot);
    g.nodes[check].pred_start = b_rs;

    link(g, rs_sub, entry);
    link(g, star_exit, check);
    link(g, check, re_sub);

    auto [ss, se] = g.add_rule("s");
    std::uint32_t sub_ref = g.add_node(NodeType::RuleRef);
    g.nodes[sub_ref].value = "_sub";
    link(g, ss, sub_ref);
    link(g, sub_ref, se);
    return g;
}

// ---- Tests ---------------------------------------------------------------

int main() {
    // --- Test 1: match a single token by type ---
    std::cout << "--- Test 1: g = NUMBER ---\n";
    {
        Parser p; p.graph = build_single_token();
        auto tree = p.parse({tok("NUMBER", "42")});
        check("matches NUMBER(42)",
              tree && tree->name == "g" &&
              tree->children.size() == 1 &&
              tree->children[0]->name == "NUMBER" &&
              tree->children[0]->value == "42");
        bool threw = false;
        try { p.parse({tok("STRING", "x")}); } catch (...) { threw = true; }
        check("rejects wrong type", threw);
    }

    // --- Test 2: NUMBER '+' NUMBER (literal by value) ---
    std::cout << "\n--- Test 2: g = NUMBER '+' NUMBER ---\n";
    {
        Parser p; p.graph = build_addition();
        auto tree = p.parse({
            tok("NUMBER", "12"),
            tok("PLUS",   "+"),
            tok("NUMBER", "34"),
        });
        check("root is 'g'", tree && tree->name == "g");
        check("3 children", tree->children.size() == 3);
        check("first child is NUMBER 12",
              tree->children[0]->name == "NUMBER" &&
              tree->children[0]->value == "12");
        check("middle child is PLUS '+' (matched by value)",
              tree->children[1]->name == "PLUS" &&
              tree->children[1]->value == "+");
        check("last child is NUMBER 34",
              tree->children[2]->name == "NUMBER" &&
              tree->children[2]->value == "34");
        std::cout << tree->pretty() << "\n";
    }

    // --- Test 3: alternatives (by value) ---
    std::cout << "\n--- Test 3: g = 'foo' | 'bar' ---\n";
    {
        Parser p; p.graph = build_alternatives();
        auto t1 = p.parse({tok("WORD", "foo")});
        check("matches foo", t1 && t1->children[0]->value == "foo");
        auto t2 = p.parse({tok("WORD", "bar")});
        check("matches bar", t2 && t2->children[0]->value == "bar");
        bool threw = false;
        try { p.parse({tok("WORD", "baz")}); } catch (...) { threw = true; }
        check("rejects 'baz'", threw);
    }

    // --- Test 4: skip tokens (WS invisible) ---
    std::cout << "\n--- Test 4: skip WS ---\n";
    {
        Parser p; p.graph = build_skip_ws();
        // WS tokens between significant tokens should be silently skipped
        auto tree = p.parse({
            tok("WS", " "),
            tok("NUMBER", "12"),
            tok("WS", "   "),
            tok("PLUS", "+"),
            tok("WS", " "),
            tok("NUMBER", "34"),
            tok("WS", " "),
        });
        check("WS tokens ignored", tree && tree->name == "g" &&
                                    tree->children.size() == 3);
        // Without WS works too
        auto t2 = p.parse({
            tok("NUMBER", "12"),
            tok("PLUS", "+"),
            tok("NUMBER", "34"),
        });
        check("works with no whitespace", t2 && t2->name == "g");
    }

    // --- Test 5: explicit reference to skip token ---
    std::cout << "\n--- Test 5: WS skip but required where referenced ---\n";
    {
        Parser p; p.graph = build_let_with_explicit_ws();
        // With whitespace between LET and IDENT: parses
        auto tree = p.parse({
            tok("LET", "let"),
            tok("WS", " "),
            tok("IDENT", "x"),
        });
        check("'let x' parses", tree && tree->name == "g");
        // Without whitespace: rejected, because WS is explicitly required
        bool threw = false;
        try {
            p.parse({
                tok("LET", "let"),
                tok("IDENT", "x"),
            });
        } catch (...) { threw = true; }
        check("'letx' rejected (WS required)", threw);
    }

    // --- Test 6: nested rules ---
    std::cout << "\n--- Test 6: expr = term '+' term ---\n";
    {
        Parser p; p.graph = build_nested();
        auto tree = p.parse({
            tok("NUMBER", "12"),
            tok("PLUS",   "+"),
            tok("NUMBER", "34"),
        });
        check("root is expr", tree && tree->name == "expr");
        check("3 children", tree->children.size() == 3);
        check("first is term(NUMBER)",
              tree->children[0]->name == "term" &&
              tree->children[0]->children.size() == 1 &&
              tree->children[0]->children[0]->value == "12");
        check("third is term(NUMBER)",
              tree->children[2]->name == "term" &&
              tree->children[2]->children[0]->value == "34");
        std::cout << tree->pretty() << "\n";
    }

    // --- Test 7: Kleene star ---
    std::cout << "\n--- Test 7: items = NUMBER ('+' NUMBER)* ---\n";
    {
        Parser p; p.graph = build_chained_plus();
        auto t1 = p.parse({tok("NUMBER", "1")});
        check("single number", t1 && t1->children.size() == 1);
        auto t2 = p.parse({
            tok("NUMBER", "1"),
            tok("PLUS", "+"), tok("NUMBER", "2"),
            tok("PLUS", "+"), tok("NUMBER", "3"),
            tok("PLUS", "+"), tok("NUMBER", "4"),
        });
        check("four numbers, three pluses", t2 &&
              t2->children.size() == 7);
        std::cout << t2->pretty() << "\n";
    }

    // --- EBNF subtract: {"a"} - ("a","a") ---
    std::cout << "\n--- EBNF `{\"a\"} - (\"a\",\"a\")` ---\n";
    {
        Parser p; p.graph = build_ebnf_subtract();
        auto t1 = p.parse({tok("A", "a")});
        check("1 'a' matches", t1 && t1->name == "s");
        auto t3 = p.parse({tok("A", "a"), tok("A", "a"), tok("A", "a")});
        check("3 'a's match", t3 && t3->name == "s");
        auto t0 = p.parse({});
        check("empty matches", t0 && t0->name == "s");
        bool threw = false;
        try {
            p.parse({tok("A", "a"), tok("A", "a")});
        } catch (...) { threw = true; }
        check("rejects 2 'a's (subtract match)", threw);
    }

    // --- Summary ---
    std::cout << "\n========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    if (failed) { std::cout << "SOME TESTS FAILED\n"; return 1; }
    std::cout << "ALL TESTS PASSED\n";
    return 0;
}
