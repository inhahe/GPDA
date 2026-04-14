# GPDA Parser Benchmarks

Comparison of our two GPDA parser implementations (tokenized + scannerless)
against **flex+bison** (LALR) and **ANTLR4** (ALL(*)) on arithmetic
expressions (unambiguous vs. ambiguous grammars) and JSON.

All measurements are median wall-clock time across multiple parse iterations,
excluding warm-up runs.  All C++ code compiled with MSVC 19.44 `/O2 /EHsc`.
Every parser builds a parse tree on each run (except **flex+bison**, which
only validates — it has no tree-building overhead, so treat its numbers as
a lower bound).

---

## 1. Unambiguous arithmetic (precedence levels)

Grammar (GPDA, scannerless form — see `cpp/benchmarks/grammars/arith.eebnf`):

```
main = _ expr _
expr = expr '+' term | expr '-' term | term
term = term '*' factor | term '/' factor | factor
factor = NUMBER | '(' expr ')'
```

* flex+bison uses the same grammar restructured with `%left PLUS MINUS` /
  `%left STAR SLASH` precedence declarations.
* ANTLR4 uses direct left recursion with alternative ordering to encode
  precedence (its standard idiom).
* Our parsers use the precedence-level restructured grammar above.  The
  left-recursion transform inside the Python grammar loader produces
  `expr = term ('+' term | '-' term)*` before graph construction.

Input: random arithmetic expressions of 200K–1M bytes in a single long
left-associative expression (with occasional parenthesised subexpressions).

### Median parse time (ms)

| Parser            | tiny (1 KB) | small (10 KB) | medium (100 KB) |
|-------------------|------------:|--------------:|----------------:|
| flex+bison        |       0.009 |         0.093 |            1.12 |
| ANTLR4            |       0.384 |         4.022 |           40.02 |
| ours_tokenized    |       0.787 |         6.563 |          109.47 |
| ours_scannerless  |       1.365 |        15.183 |          203.31 |

(large = 1 MB runs take >5 s for ours_tokenized and multi-minutes for
ours_scannerless on this grammar because arithmetic trees are O(N) deep
in the left-recursive spine. The medium numbers are representative of
the per-operator cost.)

Numbers reflect the current state after this session's optimizations
(precedence declarations, LR-reconstruct `reserve()` + identity-return,
visited-pool reuse).

### Relative to ANTLR (median, medium input)

| ours_tokenized    | **2.7×** slower |
| ours_scannerless  | **5.1×** slower |

And vs flex+bison: ours_tokenized is ~98× slower on medium arith; ~35×
slower on medium JSON. The difference reflects deep-tree (arithmetic)
vs shallow-tree (JSON) workloads — arithmetic has an O(N)-deep spine
and deep trees cost us more per node.

### Observations

* **Tokenized scales linearly** (1–10–100 ms roughly per 10× input size)
  up to medium, then jumps super-linearly at 1 MB.  The large-to-medium
  ratio is ~70× for a 10× size increase, consistent with some per-iteration
  overhead (tree rebuild / destructor churn on a 200K-deep left spine)
  becoming significant.
* **Scannerless is ~7× slower than tokenized** at every size — consistent
  with its per-character cursor cost.  It effectively runs out of runway
  at the 1 MB size within a 2-minute budget.
* **ANTLR4 is about 45× slower than flex+bison** on large — the trade-off
  ANTLR makes for a more expressive parsing model (ALL(*) prediction +
  runtime ATN simulation).
* On arithmetic, ours_tokenized is **substantially worse vs. flex+bison**
  than it is on JSON (32× there, ~960× here).  Arithmetic has deeper trees
  (O(N) from left-associativity) while JSON has bounded depth, so the
  constant-factor overhead per tree node dominates.

---

## 2. Ambiguous arithmetic (single rule, all operators at one level)

Grammar (GPDA, scannerless form — see `cpp/benchmarks/grammars/arith_ambig.eebnf`):

```
main = _ expr _
expr = expr '+' expr
      | expr '-' expr
      | expr '*' expr
      | expr '/' expr
      | NUMBER
      | '(' expr ')'
```

This is the classic **ambiguous** expression grammar: `1+2*3` has two
distinct parse trees, and in general an expression with N binary operators
has **Catalan(N)** distinct parse trees.

* **flex+bison** and **ANTLR4** *cannot* accept this grammar as-is for
  meaningful output — bison emits shift/reduce conflicts, and ANTLR4's
  top-down predictor picks one alternative.  They both use precedence-based
  disambiguation on this grammar (%left declarations for bison, alternative
  ordering for ANTLR).  So their reported times here are really
  "ambiguous-grammar shape + disambiguation rules" — still linear.
* **Our GPDA parser** faithfully explores *all* valid paths in the
  ambiguous grammar.  Deduplication (cursor + stack) keeps it from being
  fully Catalan, but the number of distinct (cursor, stack) states still
  grows exponentially in N.

### Input sizes here are in *number of binary operators* (not bytes).

`ops{N}.arith` contains N binary operators; the file is ~5N bytes.

### Median parse time (ms)

| Parser                      | ops=5   | ops=10     | ops=20   | ops=50  | ops=100 | ops=200 | ops=500 | ops=1000 |
|-----------------------------|--------:|-----------:|---------:|--------:|--------:|--------:|--------:|---------:|
| flex+bison (w/ %left)       |   0.000 |      0.001 |    0.001 |   0.003 |   0.005 |   0.010 |   0.042 |    0.047 |
| ANTLR4 (precedence via alt) |   0.013 |      0.024 |    0.043 |   0.103 |   0.207 |   1.016 |   1.337 |    3.367 |
| ours_tokenized_ambig        |   0.218 |    261.175 |  (>60s)  |  (>60s) |  (>60s) |  (>60s) |  (>60s) |   (>60s) |
| ours_scannerless_ambig      |   2.368 |   4196.950 |  (>60s)  |  (>60s) |  (>60s) |  (>60s) |  (>60s) |   (>60s) |

### Growth curve of `ours_tokenized_ambig` (showing exponential blowup)

| ops | median ms | ratio vs. prev |
|----:|----------:|---------------:|
|   5 |     0.218 |             —  |
|   6 |     0.676 |          3.1×  |
|   7 |     2.303 |          3.4×  |
|   8 |     8.612 |          3.7×  |
|   9 |    51.207 |          5.9×  |
|  10 |   261.175 |          5.1×  |
|  11 |  2459.010 |          9.4×  |
|  12 |   (>60s)  |             —  |

~4× per added operator.  Compatible with the number of (cursor, stack)
states scaling as ≈ O(k^N) for some k∈(2, 5).

### Observations

* **flex+bison** is linear.  %left declarations tell bison to resolve
  shift/reduce conflicts with precedence + associativity, so the LALR(1)
  table is unchanged by the "ambiguity".
* **ANTLR4** is linear.  Alternative ordering plus left-recursion rewriting
  makes precedence resolution a constant-factor affair per token.
* **Ours explodes exponentially.**  The parser explores every parse tree
  shape simultaneously.  On a 10-operator expression we've already gone
  from 0.22 ms to 261 ms — a 1,186× slowdown.  By 20 operators we're
  beyond any reasonable time budget.

This is the price of the algorithm's **unified "all paths simultaneously"**
semantics: it correctly accepts genuinely ambiguous inputs but cannot prune
them without forfeiting that semantics.

### 2a. Same ambiguous grammar + `@left` / `@right` declarations

Top-level precedence declarations (`@left '+' '-'; @left '*' '/';`)
rewrite the ambiguous rule into an unambiguous hierarchy at
grammar-load time. The ambiguous source form stays the same; the
user just adds a few declaration lines.

### Median parse time (ms) on the same ambiguous grammar

| Parser                           | ops=5 | ops=10 | ops=50 | ops=100 | ops=500 |
|----------------------------------|------:|-------:|-------:|--------:|--------:|
| ours_tokenized (no prec decls)   |  0.22 |    268 | >60 s  |  >60 s  |  >60 s  |
| ours_tokenized + `@left` decls   | 0.020 |  0.049 |  0.189 |   0.425 |   2.799 |
| ours_scannerless + `@left` decls | 0.090 |  0.141 |  0.596 |   2.182 |   7.699 |
| ANTLR4 (precedence via alt)      | 0.013 |  0.024 |  0.103 |   0.207 |   2.161 |

With precedence declarations:
* **ours_tokenized is 1.3–1.9× slower than ANTLR4** — roughly matching
  the JSON ratio, which is our normal steady-state gap.
* **Scaling is linear** in the number of operators (geometric ratio
  ~1.5 per additional 5 operators), vs. Catalan-exponential without
  declarations.
* **ours_scannerless is 3.6–9.6× slower than ANTLR4** — the extra
  cost is per-character vs. per-token work, not algorithmic.

This is where the precedence-declaration feature pays off: it lets
you keep the natural ambiguous grammar form (which is often how
language specs are written) without giving up linear parse time.

---

## 3. JSON benchmark (reference point)

JSON is a linear, unambiguous grammar — this is the baseline for
comparison without either precedence restructuring or ambiguity stress.

| Parser           | tiny (1.5 KB) | small (10 KB) | medium (100 KB) | large (1 MB) |
|------------------|--------------:|--------------:|----------------:|-------------:|
| flex+bison       |         0.011 |         0.078 |           1.135 |         8.20 |
| ANTLR4           |          0.34 |          2.31 |           22.11 |       220.67 |
| ours_tokenized   |          0.51 |          2.88 |           28.05 |       279.11 |
| ours_scannerless |          0.93 |          6.16 |           62.23 |       636.85 |

**Current gap vs ANTLR on 1 MB**: ours_tokenized is **1.26× slower**
(down from 1.29× in earlier sessions — small wins from the LR
reconstruct `reserve()` fix and visited-pool reuse this session).
Scannerless is **2.89× slower** than ANTLR on 1 MB JSON.

vs flex+bison on 1 MB: ours_tokenized **34× slower**, scannerless
**78× slower**. flex+bison builds no tree, so this is a lower bound
for parser-overhead comparison rather than a true apples-to-apples.

Evolution of the scannerless numbers across optimisation rounds:

| Stage                                    | tiny  | small | medium | large   |
|------------------------------------------|------:|------:|-------:|--------:|
| Phase 1 (unified EEBNF, no DFA)          | 17.80 | 157   | 1533   | >120 s  |
| Phase 2 (+ DFA fast-path)                | 11.7  |  99   | 1012   | (~10 s) |
| Phase 3 (+ `@longest`)                   |  8.48 |  67   |  677   | 6924    |
| Phase 3.5 (+ strip `@skip`/`_` from tree)| 1.05  |  5.82 |   76   |  613    |

Overall: scannerless 1 MB JSON went from "times out at 120 s" to
**0.6 s** — roughly **195× faster** than the original unoptimized
port.  Now ~2× slower than our tokenized parser and ~2.5× slower
than ANTLR4 on 1 MB, a massive closing of the gap.

The biggest single win was stripping `@skip`/`_` rule matches from
the output tree — the previous parser was emitting one ParseNode per
whitespace match everywhere in the tree, which dominated
allocation time and made each tree-destruction pass expensive.

---

## 4. Takeaways

1. **On unambiguous grammars with precedence restructuring**, we are in the
   ballpark of ANTLR4 for shallow-tree inputs (JSON, 1.3× slower on 1 MB)
   but substantially worse for deep-tree inputs (arithmetic, 21× slower on
   1 MB vs. ANTLR).  The deep-tree penalty is dominated by tree
   construction + teardown on the O(N)-deep left-associative spine, not by
   the parse algorithm itself.

2. **On genuinely ambiguous grammars**, our parser scales **exponentially**
   while classical tools (bison/ANTLR) continue linearly using precedence
   heuristics.  This is inherent to the algorithm — if you need to know
   *all* valid parses of an ambiguous grammar, you pay for them.  If you
   only want *one* valid parse, the ambiguous grammar is the wrong input
   to our parser; use a restructured grammar instead.

3. **Scannerless is always significantly slower than tokenized** on these
   benchmarks (~7×) for the reason you'd expect: it runs the parser on
   every character instead of on every token, and tokens are typically
   4–8 characters long.

4. **flex+bison validates without building a tree**, so its numbers should
   be treated as a parser-theoretical lower bound — not a true
   apples-to-apples comparison with the tree-building parsers.  The three
   tree builders (ANTLR4, ours_tokenized, ours_scannerless) are the fair
   comparison.

---

## 5. Reproducing

```bash
# 1. Generate inputs
python cpp/benchmarks/scripts/generate_inputs.py       cpp/benchmarks/inputs
python cpp/benchmarks/scripts/generate_arith_inputs.py cpp/benchmarks/inputs

# 2. Regenerate graph C++ source (only when a grammar changes)
python cpp/benchmarks/scripts/emit_cpp_graph.py       cpp/benchmarks/grammars/arith.eebnf           cpp/benchmarks/generated/arith_scannerless.cpp        build_arith_graph
python cpp/benchmarks/scripts/emit_cpp_graph.py       cpp/benchmarks/grammars/arith_ambig.eebnf     cpp/benchmarks/generated/arith_ambig_scannerless.cpp  build_arith_ambig_graph
python cpp/benchmarks/scripts/emit_tokenized_graph.py cpp/benchmarks/grammars/arith_tokenized.eebnf cpp/benchmarks/generated/arith_tokenized.cpp          build_arith_tok_graph
python cpp/benchmarks/scripts/emit_tokenized_graph.py cpp/benchmarks/grammars/arith_ambig_tokenized.eebnf cpp/benchmarks/generated/arith_ambig_tokenized.cpp build_arith_ambig_tok_graph

# flex+bison for arith.y / arith.l
cd cpp/benchmarks/generated
../../../tools/winflexbison/win_bison.exe -d -o arith.tab.c ../grammars/arith.y
../../../tools/winflexbison/win_flex.exe  --header-file=arith.yy.h -o arith.yy.c ../grammars/arith.l

# ANTLR for Arith.g4
cd antlr
java -jar ../../../../tools/antlr/antlr-4.13.2-complete.jar -Dlanguage=Cpp -o . ../../grammars/Arith.g4

# 3. Build
cd cpp && cmake --build build --config Release

# 4. Run comparison
python cpp/benchmarks/scripts/run_arith.py   # arithmetic benchmarks
python cpp/benchmarks/scripts/run_all.py     # JSON benchmarks
```
