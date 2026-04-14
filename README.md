# GPDA — a graph-walking parser with unified EPEG

GPDA (Generalized PDA) is a parser generator built around a novel graph-walking
parsing algorithm that explores all viable parse paths simultaneously with
**zero lookahead** and no backtracking, pruning via state deduplication.

The name follows the GLR/GLL convention: "generalized" = all-paths variant,
and "PDA" (pushdown automaton) is the natural recognizer for context-free
grammars. Early drafts called the *algorithm* "EPEG"; that label now
belongs to the grammar syntax below (which really is PEG-family), and
the algorithm is GPDA, since PEG implies single-parse ordered-choice
semantics that this algorithm doesn't have.

It comes in four flavours:

| Form                      | File                    | Notes                                       |
|---------------------------|-------------------------|---------------------------------------------|
| Python scannerless        | `gpda_scannerless.py`   | Characters in, parse tree out               |
| Python tokenized          | `gpda.py`               | Auto-generates a lexer; tokens in, tree out |
| C++ scannerless           | `cpp/scannerless.{hpp,cpp}` | Same semantics, compiled from an emitted Graph   |
| C++ tokenized             | `cpp/tokenized.{hpp,cpp}`   | Same semantics, tokens in, bring-your-own-lexer |

See [`SYNTAX.md`](SYNTAX.md) for the full grammar-file syntax reference,
[`ALGORITHM.md`](ALGORITHM.md) for how each EPEG feature compiles down
to the graph-walking runtime, and [`BENCHMARKS.md`](BENCHMARKS.md) for
current performance numbers.

---

## A note on provenance

> This algorithm shares aspects with Earley, GRL, and LR(0) (none of
> which I knew about when I invented it), but this one is more
> elegant, simpler, and more capable, or at least it is more than
> some of those.
>
> The basic algorithm and all syntax added to EBNF are my ideas
> (actually, I had independently reinvented most of EBNF over BNF
> myself before having heard of EBNF), but Claude wrote the code.
> The only additions to the syntax Claude added were the special
> features for whitespace handling. Claude also figured out how to
> transform left-recursive syntax to our system (happens only on the
> syntax parsing (and graph building?) stage, not a change to my
> algorithm). Claude also added a small additional function to my
> basic algorithm, "length-bounded cap", in order to support "-" in
> EBNF.
>
> I (or, actually, Claude) made two versions of this: one that uses
> a lexer like normal parsers, and one that lexes and parses using
> the same algorithm in one pass (also my idea). The second one also
> allows us to use a more unified and powerful token+rule EPEG
> format, but is of course much slower. The normal parser supports
> a similar unified format, though, by automatically generating
> tokens from a unified token+rule syntax to feed to the lexer
> (which was my idea).
>
> Claude thinks we could improve the speed by implementing
> predictive lookahead to stop pursuing dead-end paths like ANTLR
> does with ALL(*), but I didn't want it to change my basic
> algorithm. If this ever became a serious contender to other
> parsing libraries, then it would probably be worth adding it as a
> purely pragmatic optimization.
>
> ANTLR has years of optimization going for it over this program,
> but this algorithm is probably inherently somewhat slower anyway.
> I thought zero lookahead and zero lookbehind would make it faster,
> but Claude says this algorithm is inherently slower: "Expected
> asymptotic behavior."
>
> This algorithm parses a large JSON file 19%–23% slower than ANTLR.

---

## The algorithm, briefly

At each input position, the parser holds a set of **cursors** — active parse
states of the form `(graph_node, stack, captures)`. For each input token /
character:

1. **Expand**: follow epsilon transitions (rule entry/exit, splits) until
   every cursor either reaches a terminal node (a character- or
   token-matcher) or dies.
2. **Match**: terminals that match the current input advance to the next
   position; those that don't are dropped.
3. **Dedup**: cursors that converge to the same `(node, stack, captures)`
   state are collapsed to one — this is the pruning mechanism.

That's the core. No backtracking. No lookahead. No prediction. The only
way a cursor dies is by failing to match the next input; the only way the
cursor set stays bounded is by deduplication.

For left-recursive rules the loader rewrites the grammar into a right-
recursive shape with a Kleene-tail and re-folds the resulting tree left
after the parse. For ambiguous binary-operator rules, optional
`@left` / `@right` / `@nonassoc` declarations rewrite into a precedence
ladder (avoiding the Catalan blowup you'd otherwise get when the parser
honestly tries every bracketing). For the `-` subtraction operator in
EBNF mode, the loader wraps the construct in an anonymous stripped rule
and the runtime adds a **length-bounded sub-parse** to check whether B
matches the span A just consumed — the only extension to the core
algorithm that the full feature set needed.

See [`ALGORITHM.md`](ALGORITHM.md) for a full walk-through of how every
EPEG feature — terminals, sequences, alternation, quantifiers, rules,
captures, predicates, subtraction, actions, `@skip`, left-recursion
elimination, precedence declarations, regex compilation, the
`@longest` DFA fast-path — translates into either a graph shape the
core algorithm already handles or a small, well-scoped runtime
primitive. The runtime itself stays small.

---

## Why EPEG — concise grammars

Our unified EPEG format lets you put tokens and grammar rules in one
file with one syntax, use regex shorthands, and have whitespace handled
automatically. The result is about **1.6× fewer lines** than the
classical "grammar file + lexer file" split for the same language.

**JSON in GPDA's EPEG (12 lines total):**

```
ws = [ \t\n\r]* @skip @longest
json = value
value = object | array | string | number | bool | null
object = '{' (pair (',' pair)*)? '}'
pair = string ':' value
array = '[' (value (',' value)*)? ']'
string = '"' strchar* '"' @longest
strchar = [^"\\] | '\\' .
number = '-'? ('0' | [1-9] [0-9]*) ('.' [0-9]+)? ([eE] [+\-]? [0-9]+)? @longest
bool = 'true' | 'false' @longest
null = 'null' @longest
```

**Same language in ISO 14977 EBNF + a separate lex-style token file
(~19 lines total):**

```
# lexer (12 lines)
[ \t\n\r]+                                                skip
"([^"\\]|\\.)*"                                           STRING
-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+\-]?[0-9]+)?           NUMBER
true                                                      TRUE
false                                                     FALSE
null                                                      NULL
\{                                                        LBRACE
\}                                                        RBRACE
\[                                                        LBRACK
\]                                                        RBRACK
:                                                         COLON
,                                                         COMMA

# grammar (7 lines)
json    = value ;
value   = object | array | STRING | NUMBER | bool | null ;
object  = LBRACE , [pair , {COMMA , pair}] , RBRACE ;
pair    = STRING , COLON , value ;
array   = LBRACK , [value , {COMMA , value}] , RBRACK ;
bool    = TRUE | FALSE ;
null    = NULL ;
```

Line count is only part of the story. The unified form also gives you:

* **One syntax to learn** (no mental jump between regex and EBNF).
* **One file to look at** (no jumping between `.y` and `.l`).
* **No token-declaration drift** (every literal is automatically its own
  token; no missed `%token` declarations).
* **Context-aware lexing** in the scannerless form — e.g. `@mode` /
  `@atomic` / `@skip` allow different skip-rule sets to be active inside
  different grammatical contexts (so whitespace is significant inside
  string literals but not elsewhere, without a stateful lexer).

---

## Feature highlights

* **Scannerless or tokenized** — pick your trade-off. Both use the
  same one-file unified EPEG syntax (no lexer-file / grammar-file
  split either way). Scannerless additionally gives you context-aware
  lexing (different skip rules in different grammatical contexts);
  tokenized is a few times faster in steady state because it parses
  tokens, not characters.
* **Regex-style terminals** with `/pattern/flags` — flags `i` (case-
  insensitive), `x` (verbose, strip whitespace and `# comments`), `u`
  (Unicode; UTF-8 byte-level in the scannerless form, Python `re`
  Unicode flag in the tokenized form).
* **Character classes** `[abc]`, `[^abc]`, `[a-z]`, shorthand classes
  `\d \w \s`. Unicode ranges under `/u` decompose to UTF-8 byte
  patterns (`[\u4e00-\u9fff]` works).
* **Quantifiers** `* + ?` with greedy / non-greedy forms, bounded
  `{n,m}` / `{n,}` / `{,m}`.
* **Alternation** with **ordered choice** — the first alternative
  listed wins at ambiguous forks (no precedence declarations needed
  for simple disambiguation).
* **Predicates** `&(expr)` / `!(expr)` for zero-width positive /
  negative lookahead.
* **Named captures** `name:=(expr)` and **backreferences** to capture
  names (scannerless).
* **Semantic actions** `{{ python_expr }}` at the end of any alternative
  (Python only; the C++ emitters reject grammars using them).
* **Grammar-level directives**:
  `@skip` (auto-insert a whitespace-like rule between elements),
  `@atomic` (disable auto-insert for a rule),
  `@longest` (commit to longest match for a DFA-compilable rule),
  `@keyword` (append `!wordchar` to keep a keyword from matching a
   prefix of a longer identifier),
  `@mode` (partition skip rules so different grammatical contexts see
   different skip sets),
  `@left` / `@right` / `@nonassoc` (precedence declarations for binary-
   operator rules).
* **ISO 14977 EBNF mode** — `load_grammar(text, ebnf=True)` parses the
  grammar as strict ISO EBNF (`= ; , | [] {} ()`), with the `A - B`
  exception operator supported via a length-bounded sub-parse.
* **Direct and mutual left recursion** — handled by a grammar rewrite
  pass + post-hoc tree reconstruction.

---

## Project layout

```
gpda.py                         Python tokenized parser
gpda_scannerless.py             Python scannerless parser
cpp/scannerless.{hpp,cpp}       C++ scannerless parser
cpp/tokenized.{hpp,cpp}         C++ tokenized parser
cpp/benchmarks/                 Benchmark grammars, inputs, drivers, baseline tools
cpp/benchmarks/scripts/         Python → C++ graph emitters (`emit_cpp_graph.py`,
                                `emit_tokenized_graph.py`); accept `--ebnf` flag
SYNTAX.md                       Grammar-file syntax reference
ALGORITHM.md                    How EPEG features compile to graph + runtime
BENCHMARKS.md                   Current benchmark numbers vs flex+bison + ANTLR4
README.original.md              README of a different, older project in this
                                repo — kept for provenance
```

---

## Running the tests

Each Python parser's test suite runs by invoking the file as a script:

```
python gpda.py                # 109 tokenized tests
python gpda_scannerless.py    # 201 scannerless tests
```

C++ tests build via CMake:

```
cd cpp && cmake -B build && cmake --build build --config Release
./build/Release/test_scannerless.exe    # 28 tests
./build/Release/test_tokenized.exe      # 24 tests
```

Total: 362 tests across all four parsers.

---

## A design note on speed

Zero lookahead was chosen for the algorithm's elegance — the parser is
definable in a few sentences — not because it would be faster than
engineered shortcuts like ALL(\*). On real workloads our tokenized
parser is **~1.2× slower than ANTLR4** on 1 MB of JSON (down from 1.29×
after a round of micro-optimizations) and **2–3× slower on arith with
precedence declarations**. On a deeply ambiguous grammar without
precedence declarations our parser scales exponentially while ANTLR
stays linear — the price of honestly exploring every parse tree
instead of picking one ahead of time.

If someone wanted ANTLR-grade speed on serious grammars, the obvious
next step is ALL(\*)-style lookahead to prune dead paths before
exploring them — but that's a different algorithm, not an optimization
of this one, so it's out of scope for this project.
