# How EPEG features map to the graph-walking algorithm

GPDA's core is small: walk a set of cursors through a grammar graph,
matching one input symbol per step, and deduplicate cursors that
converge to the same state. All of EPEG's surface features — rules,
alternatives, quantifiers, captures, predicates, subtraction, left
recursion, auto-whitespace, ordered choice, precedence — compile
down to that core, mostly as graph shapes and mostly without any
change to the runtime.

This document explains each feature as a **graph transform**. It's
the mental model you'd need to implement a new surface feature, or
to understand why a particular grammar parses the way it does.

---

## 1. The runtime, in one page

A cursor is a tuple `(node, stack, children, captures)`:

* `node` — where we are in the grammar graph.
* `stack` — list of return frames from active rule calls; each
  frame is `(rule_id, return_links, start_pos, parent_children,
  parent_captures)`.
* `children` — persistent list of parse-tree children accumulated
  so far for the currently-active rule.
* `captures` — dict of `{name: captured_value}` visible to the
  current rule's scope (semantic-action scope).

The parser loop, per input position:

1. **Expand**: follow epsilon transitions (SPLIT / RULE_START /
   RULE_END / RULE_REF etc.) until every live cursor has either
   reached a terminal node (match-char / match-tok / matchany /
   backref) or died.
2. **Match**: if the terminal matches the current input symbol,
   advance the cursor along each of that terminal node's outgoing
   links. If it doesn't match, the cursor dies.
3. **Dedup**: two cursors with the same `(node, stack, captures)`
   are indistinguishable in all future steps; keep the first, drop
   the rest. This is the pruning mechanism.

`expand` is the only place that mutates the stack (push on
RULE_REF, pop on RULE_END) or children / captures (at RULE_END of
a non-stripped rule, a new ParseNode is created and pushed onto
the parent's children; at RULE_END of a capture rule, the capture
scope is updated).

Node types (all four parsers; scannerless has a few extras for
characters):

| Type          | Meaning                                           |
|---------------|---------------------------------------------------|
| `MATCH_CHAR`  | Match one specific character (scannerless)        |
| `MATCH_CLASS` | Match a character class (scannerless)             |
| `MATCH_ANY`   | Match any single character (scannerless)          |
| `MATCH_STR`   | Match a token by value (tokenized)                |
| `MATCH_TOK`   | Match a token by type name (tokenized)            |
| `SPLIT`       | Epsilon fan-out / fan-in (for `\|`, `*`, `+`, `?`) |
| `RULE_START`  | Epsilon entry of a rule                           |
| `RULE_END`    | Epsilon exit of a rule                            |
| `RULE_REF`    | Epsilon: push stack frame, enter another rule     |
| `PRED_AND`    | Zero-width sub-parse; succeed if it matches       |
| `PRED_NOT`    | Zero-width sub-parse; succeed if it doesn't       |
| `BACKREF`     | Match the text captured earlier under a name      |
| `ACTION`      | Run a Python expression in the current scope      |
| `SUB_CHECK_NOT` | EBNF `-`: check B didn't match the span A did   |

Each node has a `links` vector listing its outgoing edges. That's
the entire graph — no extra metadata.

---

## 2. Terminals

**String literal `'abc'`** (scannerless): chain of three
`MATCH_CHAR` nodes, one per character, linked in sequence. Each
terminal only emits a new cursor if its character matches the
input; the sequence as a whole therefore only advances if all
three match in order.

```
'abc'  →   [MATCH_CHAR 'a'] → [MATCH_CHAR 'b'] → [MATCH_CHAR 'c']
```

**String literal `'abc'`** (tokenized): one `MATCH_STR` node whose
value is the full string. Matches one token whose `value` field
equals `'abc'`. (The auto-lexer creates a token type for every
literal string seen in the grammar; at parse time, `MATCH_STR`
compares `token.value` to the literal.)

**Character class `[a-zA-Z_]`** (scannerless): one `MATCH_CLASS`
node holding a `CharClass` struct with ranges + individual chars
+ negation flag. Matches one character according to the class
rules.

**`.` (any)**: one `MATCH_ANY` node (scannerless). Matches any
single character.

**Token reference `NUMBER`** (tokenized): one `MATCH_TOK` node
whose value is the token *type name*. Matches one token whose
`type` field equals `NUMBER`.

**Regex `/pattern/flags`**: compiled at grammar-load time into the
same graph-node vocabulary above. `.` inside a regex becomes
`MATCH_ANY`; `[a-z]` becomes `MATCH_CLASS`; `a|b` becomes a `SPLIT`;
`a*` becomes a Kleene loop (see below); etc. There is no separate
"regex engine" at parse time — a regex is just a subgraph built
the same way a hand-written rule body would be.

---

## 3. Sequences and alternatives

**Sequence `a b c`**: chain the sub-graphs. `a`'s end-node's link
list is extended with `b`'s start-node; likewise `b`'s end-node
is linked to `c`'s start-node. The sequence as a whole is the pair
`(a.start, c.end)`.

**Alternation `a | b | c`**: a fresh SPLIT node fans out to each
alternative's start; each alternative's end fans into a single
join SPLIT. The alternation's `(start, end)` is that outer pair.

```
       ┌→ a.start …→ a.end ─┐
SPLIT ─┼→ b.start …→ b.end ─┼→ SPLIT(join)
       └→ c.start …→ c.end ─┘
```

**Ordered choice** falls out naturally. `expand` follows link[0]
first (via a trampoline that advances the cursor in-place without
stack-pushing), link[1] second (pushed onto the work list), etc.
So a cursor exploring the first alternative reaches any shared
downstream state strictly before a cursor exploring the second.
The dedup pass keeps the first one to arrive; the later ones are
discarded.

No priority tracking is needed for this: link ordering is already
temporal ordering in `expand`, and first-seen-wins in dedup gives
you the disambiguation rule for free.

---

## 4. Quantifiers

All quantifiers are just SPLITs with a specific link topology.
**Greedy vs. non-greedy is the order of the SPLIT's outgoing
links, nothing else.** Both variants generate identical graphs
with link order swapped; the link-order-first traversal + dedup
then gives you the disambiguation — greedy means the "match more"
path reaches any shared downstream state before the "match less"
path, so dedup keeps the greedy one.

**`a?` (optional)**: one SPLIT that goes either into `a` or
directly to the join:

```
SPLIT(entry) ──┬→ a.start …→ a.end ──┐
               └───────────────────→ SPLIT(exit)
```

SPLIT's two outgoing links are `[a.start, exit]` for greedy `?`
(try to match `a` first) and `[exit, a.start]` for non-greedy `??`
(try to skip first).

**`a*` (zero-or-more)**: one SPLIT at the entry (either match or
exit) and a back-edge from `a.end` to the entry:

```
        ┌────────────────────────────┐
        ↓                            │
SPLIT(entry) ──┬→ a.start …→ a.end ──┘
               └→ SPLIT(exit)
```

Links on the entry SPLIT: `[a.start, exit]` greedy, `[exit,
a.start]` non-greedy (`*?`). The greedy version prefers another
iteration; the non-greedy version prefers exiting.

**`a+` (one-or-more)**: mandatory `a` first, then a SPLIT after
`a.end` that either loops back to `a.start` or exits:

```
a.start …→ a.end ──→ SPLIT(loop) ──┬→ a.start (loop)
                                    └→ SPLIT(exit)
```

Loop SPLIT's links: `[a.start, exit]` for `+` (greedy — try
another iteration), `[exit, a.start]` for `+?` (non-greedy).

**`a{n,m}`** (bounded repetition): unroll `n` mandatory copies of
`a` in sequence, then `(m-n)` optional copies chained together.
Each optional copy is a `?`-style SPLIT, and the greedy vs non-
greedy link order is applied to every one of those SPLITs
uniformly. `a{n,}` is `n` mandatory copies followed by `a*`, with
the `*`'s greedy/non-greedy bit honored.

The runtime sees quantifier SPLITs as ordinary SPLITs — it doesn't
know or care whether they came from a `|`, a `?`, a `*`, a `+`,
or a `{n,m}`. The trampoline processes link[0] first and pushes
the rest onto the work list; dedup collapses convergent cursors
first-seen. That's the entire mechanism — both ordered choice at
user-level `|` *and* greedy/non-greedy disambiguation at
quantifiers are two names for the same property of the graph.

Worth noting: greedy doesn't mean "commit to matching more";
*both* alternatives are still explored. It just means the
committed-to-more cursor arrives at any shared downstream state
first, so dedup keeps it. If the "match more" path later dies
(e.g., runs out of input), the "match less" path — still alive
if it hadn't duplicated away — picks up.

---

## 5. Rules and references

Every named rule has a `RULE_START` node and a `RULE_END` node,
linked via the rule body's sub-graph.

A **reference** to a rule `foo` compiles to a `RULE_REF` node
whose `rule_id` points to `foo`. At parse time, when `expand`
encounters a `RULE_REF`:

1. Push a new `StackEntry` containing:
   * the rule id,
   * the `RULE_REF` node's `links` (these are where we return to
     after the called rule completes — i.e., the places in the
     calling rule's body that follow the `RULE_REF`),
   * the current position (for start_pos),
   * the current `children` list (saved; we'll restore it when the
     callee completes),
   * the current `captures` dict (saved for the same reason).
2. Reset `children` to empty (fresh list for the callee's own
   children) and `captures` to empty (fresh capture scope).
3. Jump to the callee's `RULE_START`.

At `RULE_END`:

1. Pop the top `StackEntry`.
2. If the rule isn't stripped: create a `ParseNode` with the
   rule's name, its accumulated children, value (see actions
   below), and push it onto the caller's restored-children list.
3. If the rule is a capture rule: add an entry to `captures`
   binding the rule name to the rule's effective value.
4. Jump along each of the saved `return_links` (one cursor per
   link, like a SPLIT fan-out).

The effect is that rules look like subroutine calls — the stack
tracks the call chain, the return-link list tells the callee
where the caller wanted to continue, and children / captures are
scoped per-call.

---

## 6. Captures and backreferences

**`name:=(expr)`**: at grammar-build time, we synthesise an
anonymous rule whose body is `expr`, mark it as a *capture* rule,
and emit a `RULE_REF` to it in place of the `:=` construct. The
rule name is `name`; the binding is registered in `capture_names`.

At parse time, `RULE_END` of a capture rule updates the returning
`captures` dict: `captures[name] = value` (where value is the
rule's action result if any, or — for scannerless — the matched
text substring, or — for tokenized — the single-child rule's
value via `$$ = $1` fallback).

**Backreference** (scannerless): a bare reference to a name that
is also a capture compiles to a `BACKREF` node, not a `RULE_REF`.
At runtime, `BACKREF` reads `captures[name]` and checks whether
the input text from the current position equals that captured
string. If so, the cursor advances by `len(captured_string)`
characters (scheduled via the parser's `deferred` map — the
cursor wakes up at that future position).

The binding is strictly the *text* that was captured, not a
re-parse of the capture rule. This keeps backreferences cheap and
terminates by construction.

---

## 7. Predicates `&(expr)` and `!(expr)`

Zero-width sub-parse. At grammar-build time, we create an
anonymous rule whose body is `expr`, wrap it in a `PRED_AND` or
`PRED_NOT` node whose `pred_start` field is the anonymous rule's
`RULE_START`, and emit the pred node in place of the `&` / `!`
construct.

At parse time, when `expand` reaches a predicate node:

1. Call `evaluate_predicate(pred_start, current_position,
   current_captures)`.
2. `evaluate_predicate` is a stripped-down version of the main
   parse loop: it starts a fresh cursor at `pred_start`, advances
   it through expand + match + dedup exactly as the outer parser
   would, and returns `true` as soon as any cursor reaches a
   completion.
3. Depending on `PRED_AND` vs `PRED_NOT` and the sub-parse result,
   either continue along the pred node's outgoing links or let the
   outer cursor die.
4. The outer cursor's position is **unchanged** — predicates are
   zero-width.

Predicates can recurse: an outer predicate can call
`evaluate_predicate` inside which a nested predicate triggers
another `evaluate_predicate`. A depth counter caps this at 50 to
catch pathological grammars.

---

## 8. EBNF exception `A - B`

Added after the fact, and the only feature that required a new
runtime primitive — a **length-bounded sub-parse**.

At grammar-build time, `A - B` compiles to:

1. A fresh anonymous rule `_sub_N`, marked as stripped (no
   ParseNode appears in the output tree).
2. The rule's body: the compiled `A` sub-graph, followed by a
   `SUB_CHECK_NOT` node whose `pred_start` points at a wrapped
   version of `B`, followed by `RULE_END`.
3. A `RULE_REF` to `_sub_N` emitted in place of the `-`
   construct.

At parse time, when `expand` reaches the `SUB_CHECK_NOT` node:

1. `stack.top().start_pos` is `A`'s start (captured when
   `RULE_REF _sub_N` pushed the frame).
2. The current position is `A`'s end.
3. Call `evaluate_predicate_bounded(pred_start, start_pos,
   end_pos, captures)`: a sub-parse that only advances within the
   exact span `[start_pos, end_pos)`, and returns true only if a
   cursor reaches completion **exactly at `end_pos`**.
4. If bounded-B matched the same span, the outer cursor dies;
   otherwise it continues through the `SUB_CHECK_NOT`'s link to
   `RULE_END`.

The bounded sub-parse is the only meaningful extension to the
core algorithm in the whole codebase. Everything else is graph
shape.

---

## 9. Semantic actions `{{ code }}`

Python only. At grammar-build time, a trailing action on an
alternative compiles to an `ACTION` node placed at the end of the
sequence.

At parse time, `expand` on `ACTION`:

1. Build a local scope from the current `captures` dict, plus
   `_text` / `_start` / `_end` (scannerless) or `_tokens` /
   `_start` / `_end` (tokenized) describing the current rule's
   span.
2. `eval(node.value, _ACTION_GLOBALS, scope)` — run the Python
   expression.
3. Stash the result in `captures['__result__']` and continue.

At `RULE_END`, the rule's effective value is taken from
`captures['__result__']` if present, else inherited from a single
non-trivial child (bison's `$$ = $1` default). That value is
propagated as the ParseNode's `value` field and as the capture
binding for `captures[rule_name]` if the rule is a capture.

The C++ emitters reject grammars containing actions — they're a
Python-only feature.

---

## 10. Grammar-level rewrites (load-time, before any cursors run)

Several features look like runtime magic but are actually
**pre-parse grammar rewrites** that turn a user-friendly form into
a simpler form the core algorithm already knows how to handle.

### 10.1. `@skip` auto-whitespace insertion

If the grammar has a `_` rule or any `@skip`-marked rule, the
loader walks every sequence in every non-atomic rule and inserts
a reference to a synthesised skip rule between consecutive
elements. For example:

```
_ = /\s+/
expr = term '+' term
```

…becomes (conceptually):

```
_ = /\s+/
expr = term _ '+' _ term
```

Atomic rules (ALL-CAPS names, `@atomic`-tagged rules, or the skip
rules themselves) aren't touched. Multiple `@skip` rules can
coexist; they're combined into a synthetic `(ws1 | ws2)*` rule
per `@mode`. Synthesised rules are marked stripped so they don't
show up in the output tree.

No runtime change — the skip rule is just another grammar rule,
compiled to its own sub-graph like everything else.

### 10.2. Left-recursion elimination

Direct left recursion (`expr = expr '+' term | term`) would
infinite-loop in a top-down parser because the RULE_REF to `expr`
on the left pushes a stack frame without consuming input. The
loader detects this and rewrites:

```
expr = expr '+' term | expr '-' term | term
```

to:

```
expr = term _expr_lr_tail*
_expr_lr_tail = '+' term | '-' term
```

This is a pure pre-parse transform — no runtime support needed.
The downside is the resulting parse tree is *right*-recursive
(flat, with tails as siblings) instead of *left*-recursive. To
restore left-associativity, after the parse completes a
`reconstruct_lr` pass folds the flat structure back left.

Mutual left recursion (A → B → A) is handled similarly: Paull's
algorithm inlines rules until only direct left recursion remains,
then the direct case is handled above.

### 10.3. Precedence declarations

`@left '+' '-'; @left '*' '/';` plus an ambiguous rule like
`expr = expr '+' expr | expr '*' expr | NUMBER` gets rewritten
into a precedence ladder:

```
expr = expr ('+' | '-') _expr_p1 | _expr_p1       (left-recursive)
_expr_p1 = _expr_p1 ('*' | '/') _expr_atom | _expr_atom
_expr_atom = NUMBER | '(' expr ')'
```

Each level is left-recursive (for `@left`), right-recursive (for
`@right`), or non-recursive (for `@nonassoc`). Left levels then
go through the left-recursion elimination above. The ambiguous
source form disappears by the time the graph is built.

This means the user writes the natural ambiguous form but gets
linear parse time — no Catalan-blowup exploration of every
possible bracketing, because there *is* only one bracketing post-
rewrite.

### 10.4. Regex compilation

`/pattern/flags` is parsed at grammar-load time by a regex
compiler that produces the same node-graph atoms as hand-written
rule bodies: `.` → `MATCH_ANY`, `[a-z]` → `MATCH_CLASS`,
`(a|b)` → `SPLIT`, `a*` → Kleene loop, and so on. Flags are
honored at compile time:

* `i` — character classes expand to include both cases; literal
  letters become a two-element class.
* `x` — whitespace and `# comments` stripped from the pattern
  before parsing.
* `u` (scannerless) — `\uHHHH` above 0x7F emits a UTF-8 byte
  sequence; `.` emits the four-alternative 1/2/3/4-byte codepoint
  matcher; character-class ranges decompose into UTF-8 byte-
  pattern alternatives via the standard range-split algorithm.

### 10.5. Token auto-generation (tokenized parser only)

The tokenized parser's `load_grammar` scans the grammar for every
literal string and every inline `/regex/` and creates a token
definition for each one. References in the rule bodies are
rewritten to token references.

```
stmt = 'if' expr 'then' stmt
```

gets auto-expanded to:

```
_lit_1 = /if/
_lit_2 = /then/
stmt = _lit_1 expr _lit_2 stmt
```

…plus a generated lexer that tokenizes input into these auto-
created token types. This preserves the one-file EPEG form for
the tokenized parser at the cost of a lexer-synthesis step.

---

## 11. Runtime extensions — the `@longest` DFA fast-path

Regular sub-rules (no recursion, no predicates, no captures) can
be compiled into a proper NFA→DFA at grammar-load time. When the
main parser encounters a `RULE_REF` to such a rule, instead of
walking the sub-graph character-by-character, it runs the DFA
over the input and produces cursors at each accept position.

`@longest` marks a DFA rule as "emit only the longest-match
cursor", replicating the maximal-munch behaviour of a traditional
lexer. Without `@longest`, an all-paths parser would keep a
cursor at every accept position — for a rule like `/\d+/` that's
one cursor per digit, which blows up downstream.

The DFA is a **separate algorithm** from the core graph walker
(classical table-driven, `trans[state*256 + byte]`); the main
parser just calls it as a subroutine for DFA-compiled rules. It
lives in the C++ parser; the Python parsers still walk the
sub-graph char-by-char (fast-enough in Python where everything is
interpreted anyway).

---

## 12. Summary — what the runtime actually needs to implement

In total, the runtime (per parser) implements:

1. The expand / match / dedup loop.
2. Stack push on RULE_REF, stack pop + ParseNode create on
   RULE_END.
3. Predicate sub-parse (a specialised expand/match/dedup loop
   that checks for completion).
4. Bounded predicate sub-parse (same, but only succeeds at a
   fixed end position) — for `-`.
5. Character / token matching (one function per terminal type).
6. Deferred-cursor map for multi-character backref consumption.
7. Captures dict propagation.
8. `eval()` for actions (Python only).
9. DFA fast-path for `@longest` rules (C++ only).

Everything else is in the grammar loader: rewrites that reduce
the full surface syntax to this small set of node types and runtime
mechanisms. That division — a small, boring runtime; a clever
loader — is the reason the algorithm stays so simple even as the
surface grows.
