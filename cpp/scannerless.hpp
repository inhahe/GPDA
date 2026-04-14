// scannerless.hpp — C++ port of gpda_scannerless.py
//
// Graph-walking scannerless parser.  Grammar is built into a Graph object
// (nodes with links).  The parser walks the graph following all viable
// paths simultaneously with zero lookahead.
#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pool.hpp"

namespace gpda {

// ============================================================================
// Persistent list (for stack and children — O(1) sharing between cursors)
// ============================================================================

template <typename T>
struct PList : gpda_pool::Refcounted<PList<T>> {
    T head;
    gpda_pool::IntrusivePtr<PList> tail;
    std::size_t length;

    PList(T h, gpda_pool::IntrusivePtr<PList> t, std::size_t len)
        : head(std::move(h)), tail(std::move(t)), length(len) {}

    static void deallocate(PList* p) noexcept {
        gpda_pool::Pool<PList>::instance().destroy(p);
    }

    // Iterative destructor to prevent stack overflow when a long chain
    // is released.  See pool.hpp for context.  Detach `tail` first, then
    // manually release the rest in a loop.  When a pool'd PList is
    // destructed via T::deallocate, its already-empty tail member's
    // destructor is a no-op, so no recursion.
    ~PList() {
        auto t = std::move(tail);
        while (t && t.is_unique()) {
            auto next = std::move(t->tail);
            t.reset();
            t = std::move(next);
        }
    }
};

template <typename T>
using PListPtr = gpda_pool::IntrusivePtr<PList<T>>;

template <typename T>
inline PListPtr<T> plist_push(PListPtr<T> tail, T value) {
    std::size_t len = tail ? tail->length : 0;
    auto* p = gpda_pool::Pool<PList<T>>::instance().make(
        std::move(value), std::move(tail), len + 1);
    return PListPtr<T>(p);
}

template <typename T>
inline PListPtr<T> plist_pop(const PListPtr<T>& list) {
    return list ? list->tail : PListPtr<T>();
}

// Convert persistent list to vector in push-order (bottom-up).
template <typename T>
inline std::vector<T> plist_to_vector(const PListPtr<T>& list) {
    std::vector<T> out;
    if (!list) return out;
    out.reserve(list->length);
    for (auto p = list.get(); p; p = p->tail.get()) out.push_back(p->head);
    std::reverse(out.begin(), out.end());
    return out;
}

// ============================================================================
// Character class
// ============================================================================

struct CharClass {
    std::vector<std::pair<char, char>> ranges;
    std::vector<char> chars;
    bool negated = false;

    bool matches(char c) const {
        bool hit = false;
        for (char ch : chars) {
            if (ch == c) { hit = true; break; }
        }
        if (!hit) {
            for (const auto& r : ranges) {
                if (r.first <= c && c <= r.second) { hit = true; break; }
            }
        }
        return negated ? !hit : hit;
    }
};

// ============================================================================
// Graph nodes
// ============================================================================

enum class NodeType : std::uint8_t {
    MatchChar,   // match one specific character
    MatchClass,  // match one character in a CharClass
    MatchAny,    // match any single character (.)
    RuleRef,     // reference to another rule (epsilon)
    Split,       // fan-out / join point (epsilon)
    RuleStart,   // rule entry (epsilon)
    RuleEnd,     // rule exit (epsilon)
    PredNot,     // !(expr) — zero-width negative lookahead
    PredAnd,     // &(expr) — zero-width positive lookahead
    BackRef,     // match text previously captured by name
    // EBNF `A - B` subtraction: reject this cursor if the sub-graph at
    // pred_start matches the exact span from stack.top().start_pos to
    // the current pos.  Used as the trailing step of an anonymous
    // stripped rule `_sub_N = A <SubCheckNot(B)>` wrapping `A - B`.
    SubCheckNot,
};

struct Node {
    NodeType type{NodeType::Split};
    char char_val = 0;
    CharClass cclass;
    std::string name;        // rule name (RuleRef, BackRef, RuleStart, RuleEnd)
    std::uint32_t pred_start = 0;       // for PredNot / PredAnd
    std::uint32_t rule_id = UINT32_MAX; // for RuleRef — resolved at finalize()
    std::vector<std::uint32_t> links;
};

struct Rule {
    std::uint32_t start = 0;
    std::uint32_t end = 0;
};

// A DFA compiled from a regular rule.  When the parser hits a RuleRef to
// a rule with a DFA, it runs the DFA directly over the input instead of
// walking the rule's sub-graph character-by-character.
//
// `trans` is a flat row-major table: `trans[state * 256 + byte]` is the
// next state, or UINT32_MAX for "no transition".
// `accept[state]` is 1 iff state is an accepting state.
// State 0 is the start state.
struct Dfa {
    std::uint32_t num_states = 0;
    std::vector<std::uint32_t> trans;
    std::vector<std::uint8_t>  accept;
    std::string rule_name;              // for parse-tree node name
    std::uint32_t rule_id = UINT32_MAX; // filled at finalize()
    bool longest = false;               // @longest: emit only the
                                         // longest accept position
                                         // instead of every accept
};

struct Graph {
    std::vector<Node> nodes;
    std::unordered_map<std::string, Rule> rules;           // used at build
    std::unordered_map<std::string, std::string> lr_meta;
    std::unordered_set<std::string> capture_names;
    // Rule names whose matches are invisible in the parse tree (auto-ws
    // `_`, user `@skip`, synthesized `_skip`).  Populated by the graph
    // builder; translated into the `rule_stripped` array at finalize().
    std::unordered_set<std::string> stripped_names;
    std::string start_rule;
    std::vector<Dfa> dfas;  // keyed independently; rule_dfa[] maps rule_id→idx

    // Fast-access arrays populated by finalize().  Indexed by rule_id.
    std::vector<std::uint32_t> rule_starts;
    std::vector<std::uint32_t> rule_ends;
    std::vector<std::string>   rule_names;
    std::vector<std::uint8_t>  rule_is_capture;
    std::vector<std::uint8_t>  rule_stripped;  // 1 = don't emit ParseNode
    // rule_dfa[rule_id] = index into `dfas`, or UINT32_MAX if the rule
    // has no DFA.  Populated in finalize().
    std::vector<std::uint32_t> rule_dfa;
    std::uint32_t              start_rule_id = UINT32_MAX;
    bool                       finalized = false;

    std::uint32_t add_node(NodeType t) {
        nodes.push_back(Node{});
        nodes.back().type = t;
        return static_cast<std::uint32_t>(nodes.size() - 1);
    }

    std::pair<std::uint32_t, std::uint32_t> add_rule(const std::string& name) {
        std::uint32_t s = add_node(NodeType::RuleStart);
        std::uint32_t e = add_node(NodeType::RuleEnd);
        nodes[s].name = name;
        nodes[e].name = name;
        rules[name] = {s, e};
        return {s, e};
    }

    // Resolve rule references to integer IDs and populate the fast-access
    // arrays.  Parser calls this lazily on first parse; callers can also
    // invoke it explicitly.  Safe to call more than once.
    void finalize() {
        if (finalized) return;
        rule_starts.clear();
        rule_ends.clear();
        rule_names.clear();
        std::unordered_map<std::string, std::uint32_t> by_name;
        by_name.reserve(rules.size());
        for (const auto& kv : rules) {
            auto id = static_cast<std::uint32_t>(rule_starts.size());
            rule_starts.push_back(kv.second.start);
            rule_ends.push_back(kv.second.end);
            rule_names.push_back(kv.first);
            by_name[kv.first] = id;
        }
        for (auto& n : nodes) {
            if (n.type == NodeType::RuleRef) {
                auto it = by_name.find(n.name);
                n.rule_id = (it != by_name.end()) ? it->second : UINT32_MAX;
            }
        }
        rule_is_capture.assign(rule_starts.size(), 0);
        for (const auto& name : capture_names) {
            auto it = by_name.find(name);
            if (it != by_name.end()) rule_is_capture[it->second] = 1;
        }
        rule_stripped.assign(rule_starts.size(), 0);
        for (const auto& name : stripped_names) {
            auto it = by_name.find(name);
            if (it != by_name.end()) rule_stripped[it->second] = 1;
        }
        rule_dfa.assign(rule_starts.size(), UINT32_MAX);
        for (std::uint32_t i = 0; i < dfas.size(); ++i) {
            auto it = by_name.find(dfas[i].rule_name);
            if (it != by_name.end()) {
                rule_dfa[it->second] = i;
                dfas[i].rule_id = it->second;
            }
        }
        auto sit = by_name.find(start_rule);
        start_rule_id = (sit != by_name.end()) ? sit->second : UINT32_MAX;
        finalized = true;
    }
};

// ============================================================================
// Parse tree
// ============================================================================

struct ParseNode : gpda_pool::Refcounted<ParseNode> {
    std::string name;
    std::string_view text;
    std::vector<gpda_pool::IntrusivePtr<ParseNode>> children;
    std::uint32_t start = 0;
    std::uint32_t end = 0;

    static void deallocate(ParseNode* p) noexcept {
        gpda_pool::Pool<ParseNode>::instance().destroy(p);
    }

    // Iterative destructor — prevents stack overflow on deep trees (e.g. a
    // left-associative arithmetic spine is O(N) deep).  We move uniquely-
    // owned descendants into a worklist and empty each one's children
    // before its ref drops to zero, so every implicit ~ParseNode call runs
    // against an already-empty children vector.
    ~ParseNode() {
        if (children.empty()) return;
        std::vector<gpda_pool::IntrusivePtr<ParseNode>> worklist;
        worklist.reserve(children.size());
        for (auto& c : children) worklist.push_back(std::move(c));
        children.clear();
        while (!worklist.empty()) {
            auto n = std::move(worklist.back());
            worklist.pop_back();
            if (n.is_unique()) {
                for (auto& c : n->children) worklist.push_back(std::move(c));
                n->children.clear();
            }
        }
    }

    std::string pretty(int indent = 0) const;
};
using ParseNodePtr = gpda_pool::IntrusivePtr<ParseNode>;

// Helper: allocate a new ParseNode via the pool and wrap in IntrusivePtr.
inline ParseNodePtr make_parse_node() {
    return ParseNodePtr(gpda_pool::Pool<ParseNode>::instance().make());
}

// ============================================================================
// Cursor state
// ============================================================================

struct StackEntry {
    std::uint32_t rule_id;
    const std::vector<std::uint32_t>* return_links;    // view into Node::links
    std::uint32_t start_pos;
    // Persistent list of the parent rule's children, kept in reverse
    // insertion order (head = most recent).  Copying is O(1); pushing is
    // O(1); converting to a vector for a ParseNode is O(n) once per rule.
    PListPtr<ParseNodePtr> parent_children;
};

using CapturesPtr =
    std::shared_ptr<const std::unordered_map<std::string, std::string>>;

struct Cursor {
    std::uint32_t node;
    PListPtr<StackEntry> stack;
    PListPtr<ParseNodePtr> children;  // reverse insertion order
    CapturesPtr captures;             // null means empty
};

// Ordered-choice disambiguation is implicit in the parser's depth-first
// link-order traversal: at every fork, link[0] is processed first and
// reaches any common state before link[1..N-1] do.  Combined with
// first-seen dedup and find_completions appending in traversal order,
// this gives ordered-choice semantics (earliest-listed alternative
// wins) without any explicit priority tracking.

// ============================================================================
// Parser
// ============================================================================

class Parser {
public:
    Graph graph;
    std::size_t max_depth = 200;

    // Parse *text* using the graph's start rule.  Throws std::runtime_error
    // on failure.  Returned tree's `text` views reference *text*, which must
    // outlive the tree.
    ParseNodePtr parse(std::string_view text);

private:
    std::string_view text_;
    int pred_depth_ = 0;

    // State key for visited / dedup sets.
    //
    // Identity is (node_id, stack_ptr, caps_ptr).  Owns IntrusivePtrs so
    // the referenced PList nodes stay alive while their pointers are in
    // the visited set — without this, pool reuse recycles a slot mid-call
    // and later states alias the freed pointer, causing incorrect
    // deduplication.
    struct StateKey {
        std::uint32_t node_id;
        PListPtr<StackEntry> stack;
        CapturesPtr caps;
    };

    // Flat vector-backed "set" — faster than std::unordered_set for the
    // small populations we see here (a few dozen items per expand_all call).
    // Linear scan + contiguous memory beats hash-map's node allocs.
    class Visited {
        std::vector<StateKey> items_;
    public:
        Visited() { items_.reserve(32); }
        void clear() noexcept { items_.clear(); }
        bool insert(StateKey k) noexcept {
            for (const auto& x : items_) {
                if (x.node_id == k.node_id
                        && x.stack.get() == k.stack.get()
                        && x.caps.get()  == k.caps.get()) return false;
            }
            items_.push_back(std::move(k));
            return true;
        }
    };

    // Pool of Visited objects, used as a stack — every expand_all /
    // find_completions iteration acquires one, uses it, and returns it.
    // Predicates that recurse into expand_all get their own from deeper
    // in the stack.  This avoids ~500K vector allocations per 100K-char
    // arith parse (the dominant cost identified via phase profiling).
    std::vector<Visited> visited_pool_;
    std::size_t visited_in_use_ = 0;

    Visited& acquire_visited() {
        if (visited_in_use_ >= visited_pool_.size()) {
            visited_pool_.emplace_back();
        }
        auto& v = visited_pool_[visited_in_use_++];
        v.clear();
        return v;
    }
    void release_visited() { --visited_in_use_; }

    std::vector<Cursor> expand_all(const std::vector<Cursor>& cursors,
                                   std::uint32_t pos);

    void expand(std::uint32_t node,
                const PListPtr<StackEntry>& stack,
                const PListPtr<ParseNodePtr>& children,
                std::uint32_t pos,
                const CapturesPtr& caps,
                std::vector<Cursor>& out,
                Visited& visited);

    bool evaluate_predicate(std::uint32_t pred_start,
                            std::uint32_t pos,
                            const CapturesPtr& caps);

    // EBNF `A - B`: True iff the sub-graph at *pred_start* matches the
    // exact text span [start_pos, end_pos).  Used by SubCheckNot.
    bool evaluate_predicate_bounded(std::uint32_t pred_start,
                                    std::uint32_t start_pos,
                                    std::uint32_t end_pos,
                                    const CapturesPtr& caps);

    std::vector<ParseNodePtr> find_completions(
        const std::vector<Cursor>& cursors, std::uint32_t pos);

    void find_completion(std::uint32_t node,
                         const PListPtr<StackEntry>& stack,
                         const PListPtr<ParseNodePtr>& children,
                         std::uint32_t pos,
                         const CapturesPtr& caps,
                         std::vector<ParseNodePtr>& out,
                         Visited& visited);

    ParseNodePtr reconstruct_lr(const ParseNodePtr& tree);

    bool char_matches(const Node& n, char ch) const;

    std::vector<Cursor> dedup(const std::vector<Cursor>& cursors);

    StateKey state_key(std::uint32_t node,
                       const PListPtr<StackEntry>& stack,
                       const CapturesPtr& caps) const {
        return {node, stack, caps};
    }
};

// ============================================================================
// Inline helpers
// ============================================================================

inline bool Parser::char_matches(const Node& n, char ch) const {
    switch (n.type) {
        case NodeType::MatchChar:  return ch == n.char_val;
        case NodeType::MatchClass: return n.cclass.matches(ch);
        case NodeType::MatchAny:   return true;
        default: return false;
    }
}

}  // namespace gpda
