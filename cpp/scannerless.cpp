// scannerless.cpp — implementation of gpda::Parser

#include "scannerless.hpp"

#include <algorithm>
#include <iostream>

namespace gpda {

// ============================================================================
// ParseNode::pretty
// ============================================================================

std::string ParseNode::pretty(int indent) const {
    std::string prefix(indent * 2, ' ');
    std::ostringstream os;
    if (children.empty()) {
        os << prefix << name << ": \"" << text << "\"";
    } else {
        os << prefix << name << ": \"" << text << "\"";
        for (const auto& c : children) {
            os << "\n" << c->pretty(indent + 1);
        }
    }
    return os.str();
}

// ============================================================================
// Parser::expand / expand_all — reach terminal nodes via epsilon transitions
// ============================================================================

std::vector<Cursor> Parser::expand_all(const std::vector<Cursor>& cursors,
                                       std::uint32_t pos) {
    std::vector<Cursor> out;
    for (const auto& c : cursors) {
        Visited& visited = acquire_visited();
        expand(c.node, c.stack, c.children, pos, c.captures, out, visited);
        release_visited();
    }
    return out;
}

void Parser::expand(std::uint32_t initial_node,
                    const PListPtr<StackEntry>& initial_stack,
                    const PListPtr<ParseNodePtr>& initial_children,
                    std::uint32_t pos,
                    const CapturesPtr& initial_caps,
                    std::vector<Cursor>& out,
                    Visited& visited) {
    // Iterative expand with an explicit work list.  Uses a trampoline:
    // linear chains of epsilon transitions (1-link SPLIT / RULE_START,
    // RULE_REF, RULE_END with one return link) advance `it` in place
    // instead of push/pop, avoiding shared_ptr refcount churn.
    //
    // Ordered-choice disambiguation is implicit: link[0] is processed
    // first by the trampoline, so its derived cursors arrive at any
    // shared state before link[1..N-1] do.  First-seen dedup later
    // keeps the link-order-preferred cursor naturally.
    struct Item {
        std::uint32_t node;
        PListPtr<StackEntry> stack;
        PListPtr<ParseNodePtr> children;
        CapturesPtr caps;
    };
    std::vector<Item> work;
    work.reserve(32);
    work.push_back({initial_node, initial_stack, initial_children,
                    initial_caps});

    while (!work.empty()) {
        Item it = std::move(work.back());
        work.pop_back();

        // Inner trampoline loop: keep advancing `it` as long as we're
        // following a linear chain with no branching.
        while (true) {
            if (it.stack && it.stack->length > max_depth) break;

            auto key = state_key(it.node, it.stack, it.caps);
            if (!visited.insert(std::move(key))) break;

            const Node& n = graph.nodes[it.node];

            if (n.type == NodeType::Split || n.type == NodeType::RuleStart) {
                if (n.links.empty()) break;
                for (std::size_t i = n.links.size(); i-- > 1; ) {
                    work.push_back({n.links[i], it.stack, it.children,
                                    it.caps});
                }
                it.node = n.links[0];
                continue;
            }

            if (n.type == NodeType::RuleRef) {
                if (n.rule_id >= graph.rule_starts.size()) {
                    throw std::runtime_error("Unknown rule: " + n.name);
                }
                // DFA fast-path: if this rule has been DFA-compiled, emit
                // the RuleRef as a pseudo-terminal and let the parse loop
                // run the DFA atomically (producing cursors at accept
                // positions via the deferred map).  Avoids walking the
                // rule's sub-graph char-by-char.
                if (n.rule_id < graph.rule_dfa.size() &&
                    graph.rule_dfa[n.rule_id] != UINT32_MAX) {
                    out.push_back({it.node, it.stack, it.children, it.caps});
                    break;
                }
                StackEntry entry;
                entry.rule_id = n.rule_id;
                entry.return_links = &n.links;
                entry.start_pos = pos;
                entry.parent_children = it.children;
                it.stack = plist_push(it.stack, std::move(entry));
                it.children = nullptr;
                it.node = graph.rule_starts[n.rule_id];
                continue;
            }

            if (n.type == NodeType::RuleEnd) {
                if (!it.stack) break;
                const StackEntry& top = it.stack->head;
                const std::string& rname = graph.rule_names[top.rule_id];

                // Stripped rules (@skip, `_`, synthetic `_skip`) don't
                // produce parse-tree nodes — parent's children carry on
                // unchanged.
                PListPtr<ParseNodePtr> new_children;
                if (graph.rule_stripped[top.rule_id]) {
                    new_children = top.parent_children;
                } else {
                    auto rule_node = make_parse_node();
                    rule_node->name = rname;
                    rule_node->text = text_.substr(top.start_pos,
                                                   pos - top.start_pos);
                    rule_node->start = top.start_pos;
                    rule_node->end = pos;
                    rule_node->children = plist_to_vector(it.children);
                    new_children = plist_push(top.parent_children,
                                               rule_node);
                }

                CapturesPtr new_caps = it.caps;
                if (graph.rule_is_capture[top.rule_id]) {
                    auto m = it.caps ? std::make_shared<std::unordered_map<
                                            std::string, std::string>>(*it.caps)
                                     : std::make_shared<std::unordered_map<
                                            std::string, std::string>>();
                    (*m)[rname] = std::string(text_.substr(top.start_pos,
                                                          pos - top.start_pos));
                    new_caps = m;
                }

                auto new_stack = plist_pop(it.stack);
                if (top.return_links->empty()) break;
                for (std::size_t i = top.return_links->size(); i-- > 1; ) {
                    work.push_back({(*top.return_links)[i], new_stack,
                                    new_children, new_caps});
                }
                it.node = (*top.return_links)[0];
                it.stack = std::move(new_stack);
                it.children = std::move(new_children);
                it.caps = std::move(new_caps);
                continue;
            }

            if (n.type == NodeType::PredNot || n.type == NodeType::PredAnd) {
                bool matched = evaluate_predicate(n.pred_start, pos, it.caps);
                bool passes = (n.type == NodeType::PredAnd && matched) ||
                              (n.type == NodeType::PredNot && !matched);
                if (!passes || n.links.empty()) break;
                for (std::size_t i = n.links.size(); i-- > 1; ) {
                    work.push_back({n.links[i], it.stack, it.children,
                                    it.caps});
                }
                it.node = n.links[0];
                continue;
            }

            if (n.type == NodeType::BackRef) {
                if (!it.caps) break;
                auto cit = it.caps->find(n.name);
                if (cit == it.caps->end()) break;
                if (!cit->second.empty()) {
                    out.push_back({it.node, it.stack, it.children, it.caps});
                    break;
                }
                // empty capture: epsilon, trampoline
                if (n.links.empty()) break;
                for (std::size_t i = n.links.size(); i-- > 1; ) {
                    work.push_back({n.links[i], it.stack, it.children,
                                    it.caps});
                }
                it.node = n.links[0];
                continue;
            }

            if (n.type == NodeType::SubCheckNot) {
                // EBNF A - B: stack.top().start_pos is A's start, pos is
                // A's end.  Reject if B matches the exact same span.
                if (!it.stack) break;
                std::uint32_t sp = it.stack->head.start_pos;
                if (evaluate_predicate_bounded(n.pred_start, sp, pos,
                                                it.caps)) {
                    break;
                }
                if (n.links.empty()) break;
                for (std::size_t i = n.links.size(); i-- > 1; ) {
                    work.push_back({n.links[i], it.stack, it.children,
                                    it.caps});
                }
                it.node = n.links[0];
                continue;
            }

            // Terminal
            out.push_back({it.node, it.stack, it.children, it.caps});
            break;
        }
    }
}

// ============================================================================
// Parser::evaluate_predicate — sub-parse for &/! predicates
// ============================================================================

bool Parser::evaluate_predicate(std::uint32_t pred_start,
                                std::uint32_t pos,
                                const CapturesPtr& caps) {
    if (++pred_depth_ > 50) { --pred_depth_; return false; }
    struct Scope { int* p; ~Scope() { --*p; } } scope{&pred_depth_};

    std::vector<Cursor> cursors = {{pred_start, nullptr, nullptr, caps}};
    // Deferred cursors scheduled at future positions by multi-char
    // backreferences inside the predicate.  Mirrors the same mechanism
    // in parse(): when a backref matches ref.size() chars, the cursor
    // is parked at pos + ref.size() and woken up when the loop reaches
    // that position.
    std::unordered_map<std::size_t, std::vector<Cursor>> deferred;

    // Immediate completion?
    if (!find_completions(cursors, pos).empty()) return true;

    for (std::size_t i = pos; i < text_.size(); ++i) {
        auto dit = deferred.find(i);
        if (dit != deferred.end()) {
            for (auto& c : dit->second) cursors.push_back(std::move(c));
            deferred.erase(dit);
            cursors = dedup(cursors);
            // A woken deferred cursor may already complete at pos i
            // (entire predicate body matched by the backref) — check
            // zero-width before consuming another char.
            if (!cursors.empty() &&
                !find_completions(cursors,
                                   static_cast<std::uint32_t>(i)).empty())
                return true;
        }
        if (cursors.empty()) {
            if (deferred.empty()) return false;
            continue;  // wait until a deferred position
        }

        auto expanded = expand_all(cursors, static_cast<std::uint32_t>(i));
        std::vector<Cursor> next;
        char ch = text_[i];
        for (const auto& e : expanded) {
            const Node& term = graph.nodes[e.node];
            if (term.type == NodeType::BackRef) {
                auto it = e.captures ? e.captures->find(term.name)
                                     : caps->end();
                if (it == caps->end()) continue;  // no capture yet
                const std::string& ref = it->second;
                std::size_t end = i + ref.size();
                if (end <= text_.size() &&
                    text_.substr(i, ref.size()) == ref) {
                    for (std::uint32_t link : term.links) {
                        deferred[end].push_back(
                            {link, e.stack, e.children, e.captures});
                    }
                }
            } else if (char_matches(term, ch)) {
                for (std::uint32_t link : term.links) {
                    next.push_back({link, e.stack, e.children, e.captures});
                }
            }
        }
        if (next.empty() && deferred.empty()) return false;
        cursors = dedup(next);
        if (!find_completions(cursors, static_cast<std::uint32_t>(i + 1))
                 .empty())
            return true;
    }
    // Drain any cursors deferred to exactly text_.size() (backref that
    // matches to end of input).
    auto dit = deferred.find(text_.size());
    if (dit != deferred.end()) {
        for (auto& c : dit->second) cursors.push_back(std::move(c));
        cursors = dedup(cursors);
    }
    return !find_completions(cursors,
                             static_cast<std::uint32_t>(text_.size()))
                .empty();
}

// ============================================================================
// Parser::evaluate_predicate_bounded — EBNF `A - B` subtraction check
// ============================================================================

bool Parser::evaluate_predicate_bounded(std::uint32_t pred_start,
                                        std::uint32_t start_pos,
                                        std::uint32_t end_pos,
                                        const CapturesPtr& caps) {
    if (++pred_depth_ > 50) { --pred_depth_; return false; }
    struct Scope { int* p; ~Scope() { --*p; } } scope{&pred_depth_};

    std::vector<Cursor> cursors = {{pred_start, nullptr, nullptr, caps}};
    if (start_pos == end_pos) {
        return !find_completions(cursors, start_pos).empty();
    }
    // Deferred cursors for multi-char backref advancement (EPEG `A - B`
    // where B contains a backref; won't fire in EBNF mode, which has no
    // backrefs).  Entries past end_pos are irrelevant — the bounded
    // check only cares whether the walk completes at exactly end_pos.
    std::unordered_map<std::size_t, std::vector<Cursor>> deferred;

    for (std::size_t i = start_pos; i < end_pos; ++i) {
        auto dit = deferred.find(i);
        if (dit != deferred.end()) {
            for (auto& c : dit->second) cursors.push_back(std::move(c));
            deferred.erase(dit);
            cursors = dedup(cursors);
        }
        if (cursors.empty()) {
            if (deferred.empty()) return false;
            continue;
        }
        auto expanded = expand_all(cursors, static_cast<std::uint32_t>(i));
        std::vector<Cursor> next;
        char ch = text_[i];
        for (const auto& e : expanded) {
            const Node& term = graph.nodes[e.node];
            if (term.type == NodeType::BackRef) {
                auto it = e.captures ? e.captures->find(term.name)
                                     : caps->end();
                if (it == caps->end()) continue;
                const std::string& ref = it->second;
                std::size_t end = i + ref.size();
                if (end <= end_pos &&
                    text_.substr(i, ref.size()) == ref) {
                    for (std::uint32_t link : term.links) {
                        deferred[end].push_back(
                            {link, e.stack, e.children, e.captures});
                    }
                }
            } else if (char_matches(term, ch)) {
                for (std::uint32_t link : term.links) {
                    next.push_back({link, e.stack, e.children, e.captures});
                }
            }
        }
        if (next.empty() && deferred.empty()) return false;
        cursors = dedup(next);
    }
    auto dit = deferred.find(end_pos);
    if (dit != deferred.end()) {
        for (auto& c : dit->second) cursors.push_back(std::move(c));
        cursors = dedup(cursors);
    }
    return !find_completions(cursors, end_pos).empty();
}

// ============================================================================
// Parser::find_completion / find_completions — detect complete parses
// ============================================================================

std::vector<ParseNodePtr> Parser::find_completions(
    const std::vector<Cursor>& cursors, std::uint32_t pos) {
    std::vector<ParseNodePtr> results;
    for (const auto& c : cursors) {
        Visited& visited = acquire_visited();
        find_completion(c.node, c.stack, c.children, pos, c.captures,
                        results, visited);
        release_visited();
    }
    return results;
}

void Parser::find_completion(std::uint32_t initial_node,
                             const PListPtr<StackEntry>& initial_stack,
                             const PListPtr<ParseNodePtr>& initial_children,
                             std::uint32_t pos,
                             const CapturesPtr& initial_caps,
                             std::vector<ParseNodePtr>& out,
                             Visited& visited) {
    struct Item {
        std::uint32_t node;
        PListPtr<StackEntry> stack;
        PListPtr<ParseNodePtr> children;
        CapturesPtr caps;
    };
    std::vector<Item> work;
    work.reserve(32);
    work.push_back({initial_node, initial_stack, initial_children,
                    initial_caps});

    while (!work.empty()) {
        Item it = std::move(work.back());
        work.pop_back();

        while (true) {
            auto key = state_key(it.node, it.stack, it.caps);
            if (!visited.insert(std::move(key))) break;

            const Node& n = graph.nodes[it.node];

            if (n.type == NodeType::Split || n.type == NodeType::RuleStart) {
                if (n.links.empty()) break;
                for (std::size_t i = n.links.size(); i-- > 1; ) {
                    work.push_back({n.links[i], it.stack, it.children,
                                    it.caps});
                }
                it.node = n.links[0];
                continue;
            }

            if (n.type == NodeType::RuleRef) {
                if (n.rule_id >= graph.rule_starts.size()) break;
                // DFA fast-path for completion (see earlier comment).
                if (n.rule_id < graph.rule_dfa.size() &&
                    graph.rule_dfa[n.rule_id] != UINT32_MAX) {
                    const Dfa& dfa = graph.dfas[graph.rule_dfa[n.rule_id]];
                    if (!dfa.accept.empty() && dfa.accept[0]) {
                        PListPtr<ParseNodePtr> new_children;
                        if (graph.rule_stripped[n.rule_id]) {
                            new_children = it.children;
                        } else {
                            auto rule_node = make_parse_node();
                            rule_node->name = dfa.rule_name;
                            rule_node->text = text_.substr(pos, 0);
                            rule_node->start = pos;
                            rule_node->end   = pos;
                            new_children = plist_push(it.children, rule_node);
                        }
                        if (n.links.empty()) break;
                        for (std::size_t i = n.links.size(); i-- > 1; ) {
                            work.push_back({n.links[i], it.stack,
                                            new_children, it.caps});
                        }
                        it.node = n.links[0];
                        it.children = std::move(new_children);
                        continue;
                    }
                    break;  // no empty match available; dead end here
                }
                StackEntry entry;
                entry.rule_id = n.rule_id;
                entry.return_links = &n.links;
                entry.start_pos = pos;
                entry.parent_children = it.children;
                it.stack = plist_push(it.stack, std::move(entry));
                it.children = nullptr;
                it.node = graph.rule_starts[n.rule_id];
                continue;
            }

            if (n.type == NodeType::PredNot || n.type == NodeType::PredAnd) {
                bool matched = evaluate_predicate(n.pred_start, pos, it.caps);
                bool passes = (n.type == NodeType::PredAnd && matched) ||
                              (n.type == NodeType::PredNot && !matched);
                if (!passes || n.links.empty()) break;
                for (std::size_t i = n.links.size(); i-- > 1; ) {
                    work.push_back({n.links[i], it.stack, it.children,
                                    it.caps});
                }
                it.node = n.links[0];
                continue;
            }

            if (n.type == NodeType::BackRef) {
                if (!it.caps) break;
                auto cit = it.caps->find(n.name);
                if (cit == it.caps->end() || !cit->second.empty()) break;
                if (n.links.empty()) break;
                for (std::size_t i = n.links.size(); i-- > 1; ) {
                    work.push_back({n.links[i], it.stack, it.children,
                                    it.caps});
                }
                it.node = n.links[0];
                continue;
            }

            if (n.type == NodeType::SubCheckNot) {
                if (!it.stack) break;
                std::uint32_t sp = it.stack->head.start_pos;
                if (evaluate_predicate_bounded(n.pred_start, sp, pos,
                                                it.caps)) {
                    break;
                }
                if (n.links.empty()) break;
                for (std::size_t i = n.links.size(); i-- > 1; ) {
                    work.push_back({n.links[i], it.stack, it.children,
                                    it.caps});
                }
                it.node = n.links[0];
                continue;
            }

            if (n.type == NodeType::RuleEnd) {
                if (it.stack) {
                    const StackEntry& top = it.stack->head;
                    const std::string& rname = graph.rule_names[top.rule_id];

                    PListPtr<ParseNodePtr> new_children;
                    if (graph.rule_stripped[top.rule_id]) {
                        new_children = top.parent_children;
                    } else {
                        auto rule_node = make_parse_node();
                        rule_node->name = rname;
                        rule_node->text = text_.substr(top.start_pos,
                                                       pos - top.start_pos);
                        rule_node->start = top.start_pos;
                        rule_node->end = pos;
                        rule_node->children = plist_to_vector(it.children);
                        new_children = plist_push(top.parent_children,
                                                   rule_node);
                    }

                    CapturesPtr new_caps = it.caps;
                    if (graph.rule_is_capture[top.rule_id]) {
                        auto m = it.caps ? std::make_shared<std::unordered_map<
                                                std::string, std::string>>(*it.caps)
                                         : std::make_shared<std::unordered_map<
                                                std::string, std::string>>();
                        (*m)[rname] = std::string(text_.substr(top.start_pos,
                                                              pos - top.start_pos));
                        new_caps = m;
                    }

                    auto new_stack = plist_pop(it.stack);
                    if (top.return_links->empty()) break;
                    for (std::size_t i = top.return_links->size(); i-- > 1; ) {
                        work.push_back({(*top.return_links)[i], new_stack,
                                        new_children, new_caps});
                    }
                    it.node = (*top.return_links)[0];
                    it.stack = std::move(new_stack);
                    it.children = std::move(new_children);
                    it.caps = std::move(new_caps);
                    continue;
                } else {
                    auto result = make_parse_node();
                    result->name = n.name;
                    result->text = text_.substr(0, pos);
                    result->start = 0;
                    result->end = pos;
                    result->children = plist_to_vector(it.children);
                    out.push_back(result);
                    break;
                }
            }

            // MatchChar / MatchClass / MatchAny — dead end
            break;
        }
    }
}

// ============================================================================
// Parser::dedup
// ============================================================================

std::vector<Cursor> Parser::dedup(const std::vector<Cursor>& cursors) {
    // First-seen wins.  Combined with link-order traversal in expand
    // (link[0] processed first via trampoline), this implements
    // ordered-choice disambiguation: link[0]-derived cursors arrive
    // at any shared state before link[1..N-1] do, so they're the ones
    // kept.
    Visited seen;
    std::vector<Cursor> out;
    out.reserve(cursors.size());
    for (const auto& c : cursors) {
        if (seen.insert(state_key(c.node, c.stack, c.captures))) {
            out.push_back(c);
        }
    }
    return out;
}

// ============================================================================
// Parser::reconstruct_lr — fold right-recursive tree into left-associative
// ============================================================================

ParseNodePtr Parser::reconstruct_lr(const ParseNodePtr& tree) {
    if (!tree || tree->children.empty()) return tree;

    // Recurse on children.  If no child was rebuilt and this rule
    // isn't LR, return the input tree unchanged — avoids allocating
    // copies of non-LR subtrees.
    std::vector<ParseNodePtr> new_children;
    new_children.reserve(tree->children.size());
    bool any_changed = false;
    for (const auto& c : tree->children) {
        auto nc = reconstruct_lr(c);
        if (nc.get() != c.get()) any_changed = true;
        new_children.push_back(std::move(nc));
    }

    auto it = graph.lr_meta.find(tree->name);
    if (it == graph.lr_meta.end()) {
        if (!any_changed) return tree;
        auto n = make_parse_node();
        n->name = tree->name;
        n->text = tree->text;
        n->children = std::move(new_children);
        n->start = tree->start;
        n->end = tree->end;
        return n;
    }

    const std::string& tail_name = it->second;
    std::vector<ParseNodePtr> base, tails;
    for (const auto& c : new_children) {
        if (c->name == tail_name) tails.push_back(c);
        else                       base.push_back(c);
    }

    if (tails.empty()) {
        auto n = make_parse_node();
        n->name = tree->name;
        n->text = tree->text;
        n->children = std::move(base);
        n->start = tree->start;
        n->end = tree->end;
        return n;
    }

    auto cur = make_parse_node();
    cur->name = tree->name;
    cur->children = base;
    cur->start = base.empty() ? tree->start : base.front()->start;
    cur->end   = base.empty() ? tree->start : base.back()->end;
    for (const auto& t : tails) {
        auto nxt = make_parse_node();
        nxt->name = tree->name;
        nxt->children.reserve(1 + t->children.size());
        nxt->children.push_back(cur);
        for (const auto& tc : t->children) nxt->children.push_back(tc);
        nxt->start = cur->start;
        nxt->end   = t->end;
        cur = nxt;
    }
    cur->text = tree->text;
    return cur;
}

// ============================================================================
// Parser::parse — main entry point
// ============================================================================

ParseNodePtr Parser::parse(std::string_view text) {
    text_ = text;
    graph.finalize();
    if (graph.start_rule_id >= graph.rule_starts.size()) {
        throw std::runtime_error("No start rule: " + graph.start_rule);
    }
    std::uint32_t start_node = graph.rule_starts[graph.start_rule_id];
    std::size_t n = text.size();

    std::vector<Cursor> cursors;
    cursors.push_back({start_node, nullptr, nullptr, nullptr});

    // Cursors scheduled for future positions (from multi-char backrefs)
    std::unordered_map<std::size_t, std::vector<Cursor>> deferred;
    std::uint32_t furthest = 0;

    for (std::size_t pos = 0; pos < n; ++pos) {
        auto dit = deferred.find(pos);
        if (dit != deferred.end()) {
            for (auto& c : dit->second) cursors.push_back(std::move(c));
            deferred.erase(dit);
            cursors = dedup(cursors);
        }

        if (cursors.empty()) continue;
        furthest = static_cast<std::uint32_t>(pos);
        char ch = text[pos];

        // Two-stage expansion loop: expand to terminals, then process any
        // DFA RuleRef terminals (which may produce empty-match cursors
        // that need re-expansion).  Non-DFA terminals are collected and
        // matched against the current char below.
        std::vector<Cursor> to_process = std::move(cursors);
        std::vector<Cursor> terminals;
        while (!to_process.empty()) {
            auto expanded = expand_all(to_process,
                                       static_cast<std::uint32_t>(pos));
            std::vector<Cursor> empty_continuations;
            for (const auto& e : expanded) {
                const Node& term = graph.nodes[e.node];
                if (term.type == NodeType::RuleRef &&
                    term.rule_id < graph.rule_dfa.size() &&
                    graph.rule_dfa[term.rule_id] != UINT32_MAX) {
                    const Dfa& dfa = graph.dfas[graph.rule_dfa[term.rule_id]];
                    const bool stripped =
                        term.rule_id < graph.rule_stripped.size() &&
                        graph.rule_stripped[term.rule_id];

                    auto build_leaf_children =
                        [&](const PListPtr<ParseNodePtr>& base,
                            std::size_t from, std::size_t to)
                            -> PListPtr<ParseNodePtr> {
                        if (stripped) return base;
                        auto leaf = make_parse_node();
                        leaf->name  = dfa.rule_name;
                        leaf->text  = text.substr(from, to - from);
                        leaf->start = static_cast<std::uint32_t>(from);
                        leaf->end   = static_cast<std::uint32_t>(to);
                        return plist_push(base, leaf);
                    };

                    // Empty match: cursor continues at same position; must
                    // be re-expanded in this iteration so it can match the
                    // current char.
                    if (!dfa.accept.empty() && dfa.accept[0]) {
                        auto new_children =
                            build_leaf_children(e.children, pos, pos);
                        for (std::uint32_t link : term.links) {
                            empty_continuations.push_back(
                                {link, e.stack, new_children, e.captures});
                        }
                    }
                    // Run the DFA forward from pos; each accept position
                    // schedules a cursor in the deferred map for that
                    // future position.  If the DFA is @longest-flagged
                    // we only emit a cursor for the longest accept.
                    std::uint32_t state = 0;
                    std::size_t i = pos;
                    std::size_t last_accept = dfa.longest ? (std::size_t)-1
                                                          : 0;
                    while (i < n) {
                        std::uint32_t c = static_cast<std::uint8_t>(text[i]);
                        std::uint32_t nxt = dfa.trans[state * 256u + c];
                        if (nxt == UINT32_MAX) break;
                        state = nxt;
                        ++i;
                        if (dfa.accept[state]) {
                            if (dfa.longest) {
                                last_accept = i;
                                continue;
                            }
                            auto new_children =
                                build_leaf_children(e.children, pos, i);
                            for (std::uint32_t link : term.links) {
                                deferred[i].push_back(
                                    {link, e.stack, new_children,
                                     e.captures});
                            }
                        }
                    }
                    if (dfa.longest && last_accept != (std::size_t)-1) {
                        auto new_children =
                            build_leaf_children(e.children, pos, last_accept);
                        for (std::uint32_t link : term.links) {
                            deferred[last_accept].push_back(
                                {link, e.stack, new_children, e.captures});
                        }
                    }
                } else {
                    terminals.push_back(e);
                }
            }
            to_process = std::move(empty_continuations);
        }

        std::vector<Cursor> next;
        next.reserve(terminals.size());
        for (const auto& e : terminals) {
            const Node& term = graph.nodes[e.node];
            if (term.type == NodeType::BackRef) {
                if (!e.captures) continue;
                auto cit = e.captures->find(term.name);
                if (cit == e.captures->end()) continue;
                const std::string& ref = cit->second;
                std::size_t end = pos + ref.size();
                if (end <= n && text.substr(pos, ref.size()) == ref) {
                    for (std::uint32_t link : term.links) {
                        deferred[end].push_back(
                            {link, e.stack, e.children, e.captures});
                    }
                }
            } else if (char_matches(term, ch)) {
                for (std::uint32_t link : term.links) {
                    next.push_back({link, e.stack, e.children, e.captures});
                }
            }
        }

        cursors = dedup(next);
    }

    // Include cursors arriving at the end via backref
    auto dit = deferred.find(n);
    if (dit != deferred.end()) {
        for (auto& c : dit->second) cursors.push_back(std::move(c));
        cursors = dedup(cursors);
    }

    auto completions = find_completions(cursors,
                                         static_cast<std::uint32_t>(n));
    if (completions.empty()) {
        std::ostringstream os;
        if (furthest < n) {
            os << "Parse failed at position " << furthest
               << " (char '" << text[furthest] << "')";
        } else {
            os << "Parse failed at end of input (position " << n << ")";
        }
        throw std::runtime_error(os.str());
    }

    // Ordered choice: completions are returned in find_completion's
    // depth-first traversal order (link[0] first), so completions[0] is
    // already the link-order-preferred parse.
    ParseNodePtr tree = completions[0];
    if (!graph.lr_meta.empty()) tree = reconstruct_lr(tree);
    return tree;
}

}  // namespace gpda
