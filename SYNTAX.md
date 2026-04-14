# EEBNF Syntax Reference

This document describes the grammar syntax accepted by the two GPDA parser
implementations:

- **`gpda.py`** — tokenized parser (runs on a token stream)
- **`gpda_scannerless.py`** — scannerless parser (runs directly on characters)

Both share the core GPDA algorithm (zero lookahead, graph-walking, all
viable paths followed simultaneously). They differ in whether the input
is pre-tokenized.

As of the unification pass, both parsers accept the same terminal
syntaxes — `/regex/flags`, string literals, `[...]` character classes,
and `.` — and the same `@skip` marker.  A grammar that uses only the
shared features will parse on either implementation.

---

## Table of contents

- [Shared syntax](#shared-syntax)
- [Tokenized syntax (`gpda.py`)](#tokenized-syntax-gpdapy)
  - [Formal EBNF](#formal-ebnf-tokenized)
  - [Whitespace handling rationale](#whitespace-handling-tokenized)
- [Scannerless syntax (`gpda_scannerless.py`)](#scannerless-syntax-gpda_scannerlesspy)
  - [Formal EBNF](#formal-ebnf-scannerless)
  - [Auto-`_` rule rationale](#auto-_-rule-rationale)
- [Common operators](#common-operators)
- [Operator precedence (`@left` / `@right` / `@nonassoc`)](#operator-precedence-left--right--nonassoc)
- [Feature matrix](#feature-matrix)
- [ISO 14977 EBNF mode](#iso-14977-ebnf-mode)

---

## Shared syntax

Both parsers use the same overall grammar file structure:

```
# Comments start with #
start <rule_name>

rule_name = expression
other_rule = ...
```

Comments run from `#` to end of line. Blank lines between rules are
allowed anywhere. The `start` declaration names the entry rule; if
omitted, the first defined rule is used.

### Alternatives and sequences

```
rule = alt1 | alt2 | alt3           # alternation
rule = elem1 elem2 elem3             # sequence (space-separated)
rule = elem1 elem2
     | alt2                          # alternatives can span lines
```

### Grouping and modifiers

```
rule = (a b | c d) 'x'      # grouped alternatives
rule = a b*                  # b zero or more times
rule = a b+                  # b one or more times
rule = a b?                  # b optional
rule = a b{3}                # b exactly three times
rule = a b{2,5}              # b between 2 and 5 times
```

### Named captures

```
rule = 'prefix' val:=(body) 'suffix'
```

The text matched by `body` is captured under the name `val`. In the
resulting parse tree, it appears as a child node named `val`.

### Subtraction (`A - B`)

`A - B` matches exactly what `A` matches, but rejects the match if `B`
would *also* match the same span (same start, same end). It's the
standard ISO 14977 EBNF exception operator, usable in both EEBNF and
EBNF modes and in both parsers.

```
identifier = (letter letter*) - keyword      # identifier, but not a keyword
keyword    = 'if' | 'then' | 'else'
letter     = [a-zA-Z]
```

Precedence: `-` binds looser than modifiers and tighter than
juxtaposition. `A* - B` means `(A*) - B`; `A , B - C` means
`A , (B - C)`. Use parentheses when subtracting a longer right-hand
side, e.g. `A - (B C)`.

At compile time, `A - B` expands to an anonymous stripped rule
`_sub_N = A <SubCheckNot(B)>`. The `SubCheckNot` node, at parse time,
runs `B` as a length-bounded sub-parse over the exact span that `A`
just matched; if `B` reaches a completion at that span's end, the
cursor is rejected.

---

## Tokenized syntax (`gpda.py`)

The tokenized parser operates on a token stream. Tokens are produced by
a lexer that can be either auto-generated from the grammar or supplied
externally.

### Terminal types

| Syntax        | Meaning                                         |
|---------------|-------------------------------------------------|
| `'text'`      | Match a token whose **value** equals `text`     |
| `"text"`      | Same (both quote styles allowed)                |
| `UPPERCASE`   | Match a token whose **type** equals `UPPERCASE` |
| `/regex/flags`| Inline regex — auto-generates a lexer token     |
| `lowercase`   | Reference to a grammar rule                     |

Regex flags after the closing `/` are optional; currently supported:
`i` for case-insensitive matching.

### Token-shaped rules

When a rule's body is exactly one bare `/regex/`, the rule **is** a
token definition:

```
NUMBER = /[0-9]+/                # defines a token called NUMBER
IDENT  = /[a-zA-Z_][a-zA-Z_0-9]*/
```

These can be referenced elsewhere as `NUMBER` / `IDENT`.

### Literal strings auto-generate tokens too

Every literal string (`'foo'`, `"="`, etc.) in the grammar produces a
corresponding lexer token. This means a grammar file is fully
self-contained — no external lexer required.

### Skip tokens (whitespace handling)

Adding the `@skip` marker to a token-shaped rule makes that token
**invisible by default** but still available when explicitly referenced:

```
_ws = /[ \t]+/ @skip
NUMBER = /[0-9]+/

expr = NUMBER '+' NUMBER          # whitespace skipped automatically
keyword = LET _ws IDENT           # whitespace REQUIRED here
LET = /let/
IDENT = /[a-zA-Z_][a-zA-Z_0-9]*/
```

`_ws` tokens are produced by the lexer but ignored during parsing
unless an active rule position specifically expects `_ws`.

### Named captures

```
assign = IDENT '=' val:=(NUMBER | STRING)
```

`val` appears in the parse tree as a named child.

### Predicates

```
id = !(keyword !/[a-zA-Z_0-9]/) IDENT
```

- `&(expr)` — zero-width; succeeds if `expr` would match
- `!(expr)` — zero-width; succeeds if `expr` would not match

Both are evaluated by a recursive sub-parse from the current token
position. They consume no input on success.

### Left recursion

Both direct and indirect (mutual) left recursion are accepted and
transformed at build time. The parse tree comes back left-associative.

```
expr = expr '+' term | term       # direct — works
A = B 'x' | 'y'                   # mutual — works
B = A 'z' | 'w'
```

### Formal EBNF (tokenized) <a id="formal-ebnf-tokenized"></a>

```ebnf
(* Meta-grammar describing the EEBNF format accepted by gpda.py *)

grammar      = { comment | newline } { decl } ;
decl         = start_decl | rule ;
start_decl   = "start" , NAME , newline ;
rule         = NAME , "=" , alternatives , [ "@skip" ] , newline ;

alternatives = sequence , { { newline } , "|" , sequence } ;
sequence     = { element } , [ ACTION ] ;
element      = atom , [ modifier ] , [ "-" , atom , [ modifier ] ] ;

atom         = STRING
             | REGEX
             | capture
             | NAME                              (* rule or token ref *)
             | "(" , alternatives , ")"
             | "&" , atom
             | "!" , atom ;

capture      = NAME , ":=" , ( atom | "(" , alternatives , ")" ) ;

modifier     = "*" | "*?" | "+" | "+?" | "?" | "??"
             | "{" , NUMBER , [ "," , NUMBER ] , "}" , [ "?" ] ;

(* Lexical tokens *)
STRING       = "'" , { any_char - "'" } , "'"
             | '"' , { any_char - '"' } , '"' ;
REGEX        = "/" , { escaped_char | any_char - "/" } , "/" , { "a".."z" } ;
ACTION       = "{{" , { any_char - "}}" } , "}}" ;
NAME         = ("a".."z" | "A".."Z" | "_") , { ("a".."z" | "A".."Z" | "0".."9" | "_") } ;
NUMBER       = "0".."9" , { "0".."9" } ;
comment      = "#" , { any_char - newline } ;
```

Convention: names that are **ALL_CAPS** (per `NAME.isupper()`) are
treated as token references; other names are rule references. The
`@skip` directive (suffix on a rule definition) marks a token as
skippable.  The `-` subtraction operator (see below) binds looser
than modifiers and tighter than juxtaposition.  The trailing
`{{ … }}` action attaches to the alternative's sequence and
supplies its semantic value.

### Whitespace handling (tokenized) <a id="whitespace-handling-tokenized"></a>

The tokenized parser supports whitespace through **two complementary
features**: the `skip` marker and explicit reference.

#### The problem

In traditional parser generators (PLY, Lex/Yacc, etc.) whitespace is
handled by a single global ignore list — characters that the lexer
silently consumes. This leads to a subtle bug:

```python
t_ignore = ' \t'
t_LAMBDA_L = r'l\s+'          # 'l' followed by whitespace
t_VARIABLE = r'[a-zA-Z_][a-zA-Z0-9_]*'
```

On input `"l foo"`, `t_ignore` eats the space *before* `t_LAMBDA_L`
gets a chance to match, so `t_LAMBDA_L` can never fire. The keyword
`l` ends up indistinguishable from a variable named `l`. You can't
opt out of the ignore list from within specific rules.

#### The solution: `skip` + explicit references

**`skip`** marks a token as invisible by default. Its tokens are still
produced by the lexer but are silently skipped during parsing as long
as no active cursor is specifically waiting for them:

```
_ws = /[ \t]+/ @skip
expr = NUMBER '+' NUMBER          # _ws is ignored between tokens
```

**Explicit reference** makes a skip token mandatory at that position:

```
LET _ws IDENT                     # whitespace is required here
```

This resolves the dilemma without touching the lexer: per-location
whitespace control is expressed in the grammar itself. Multiple skip
tokens can coexist (e.g. skip spaces but keep newlines significant):

```
_ws  = /[ \t]+/ skip             # spaces/tabs: skipped
_nl  = /\n/                      # newlines: significant

stmt = IDENT '=' expr _nl
```

Here `_nl` is **not** `skip`-marked, so every statement must end with
a real newline — perfect for a line-oriented language.

#### Implementation

During the main parse loop:

```python
if token.type in self.skip_types:
    # keep the previous cursor set as a "was-ignored" alternative
    next_cursors.extend(cursors)
```

So consuming the skip token and ignoring it are both allowed; the
cursor set diverges into both possibilities and the parser naturally
prunes whichever is inconsistent.

---

## Scannerless syntax (`gpda_scannerless.py`)

The scannerless parser operates directly on character input — no
separate lexer stage exists. Every grammar rule matches characters;
tokens as a concept don't exist here.

### Terminal types

| Syntax         | Meaning                                  |
|----------------|------------------------------------------|
| `'text'`       | Match the exact character sequence       |
| `"text"`       | Same                                     |
| `[chars]`      | Character class (matches one character)  |
| `[^chars]`     | Negated character class                  |
| `[a-z]`        | Character range                          |
| `[\d\w\s]`     | Shorthand classes inside `[...]`         |
| `.`            | Any single character                     |
| `/regex/flags` | Inline regex — compiles to char-level graph |
| `rulename`     | Reference to a rule (or backref)         |

Inside string literals and character classes, the following escape
sequences are recognised: `\n`, `\t`, `\r`, `\f`, `\v`, `\0`, `\\`,
`\'`, `\"`, `\]`, `\-`.

### Inline regex

The `/regex/` form gives you the full regex feature set in scannerless
grammars.  Supported:

| Syntax          | Meaning                                          |
|-----------------|--------------------------------------------------|
| literal char    | Matches exactly that char                        |
| `.`             | Any single character                             |
| `\t \n \r \f \v \0` | Control-character escapes                    |
| `\xHH`          | Two-digit hex escape                             |
| `\uHHHH`        | Four-digit hex escape                            |
| `\\`, `\/`, etc.| Literal backslash / slash / any single char      |
| `\d \D \w \W \s \S` | POSIX-style shorthand classes                |
| `[abc]`, `[^abc]`, `[a-z]` | Character class (incl. `\d \w \s`)    |
| `(...)`         | Non-capturing group                              |
| `(?:...)`       | Same (explicit non-capturing — same behaviour)   |
| `|`             | Alternation                                      |
| `*`, `+`, `?`   | Greedy quantifiers                               |
| `*?`, `+?`, `??`| Non-greedy quantifiers                           |
| `{n}`, `{n,}`, `{n,m}` | Bounded repetition                        |
| `{n,m}?`        | Non-greedy bounded repetition                    |
| `/…/i`          | Case-insensitive flag (letters in the pattern)   |
| `/…/x`          | Verbose: unescaped whitespace and `# comments` stripped |
| `/…/u`          | Unicode: `\uHHHH` and `.` emit UTF-8 byte sequences |

Flags combine: `/…/iux` etc.

**`x` flag** (verbose) lets you write padded patterns with comments.
Inside `[…]` brackets nothing is stripped; outside, unescaped
whitespace (spaces, tabs, newlines… but see below) and `# …` comments
are discarded. Escape a space (`\ `) or `\#` to match it literally.
Note the bootstrap lexer doesn't accept newlines inside `/…/`, so
multi-line patterns aren't usable from a grammar file today — `x` is
still handy for inline spacing.

**`u` flag** (Unicode) makes `\uHHHH` compile to the UTF-8 byte
sequence of the code point (when `HHHH > 127`), and `.` match one
full UTF-8 code point (1-4 bytes). This is byte-level — the C++
parser matches UTF-8 input natively. The Python parser, which
operates on `str` (code points), only matches under `u` when the
input is a bytes-as-latin-1 `str` (e.g., `text.encode('utf-8').decode('latin-1')`).
Without `u`, `\uHHHH` still works for code points ≤ 127; for higher
code points Python matches them as single str chars, while C++
matches only the low byte (a known per-parser asymmetry).

Character classes under `u` are Unicode-aware — ranges like
`[\u00a0-\u07ff]` or `[\u4e00-\u9fff]` (CJK) are decomposed into
the right set of UTF-8 byte patterns at compile time, so a single
class can match any code-point in the range regardless of how many
UTF-8 bytes the code point takes. Negated classes with non-ASCII
content aren't supported (complementing a code-point set in UTF-8
byte space needs more machinery than we have); declare the
complement explicitly as an alternation if you need that.

Multi-line patterns are now allowed inside `/.../` — useful with
`x` for readable regex:

```
num = /
    -?            # optional sign
    [0-9]+        # digits
    (\.[0-9]+)?   # optional fractional
/x
```

The trade-off: a forgotten closing `/` will swallow lines until the
next `/` in the file, giving a worse error message than before. No
way around that at the lexer level.

Not supported inside `/regex/` (use grammar-level equivalents instead):
- Anchors `^` `$` `\b` — whole texts are parsed start-to-end already
- Lookarounds — use `&` and `!` at grammar level
- Named groups — use `name:=(...)` at grammar level
- Backreferences — use the grammar-level backref mechanism

### Named captures and backreferences

A bare reference to a capture name inside the same rule is a
**backreference**, not a rule call — it matches the exact text that
was captured earlier. Captures shadow rules within their defining
scope:

```
string = q:=(['"]) (!q .)* q      # q at end matches the opening quote
heredoc = '<<' tag:=([a-zA-Z]+) '\n' (!('\n' tag) .)* '\n' tag
```

This is implemented by having each cursor carry a `captures` dict and
by having backreference nodes match variable-length text via deferred
cursors at future positions.

### Predicates

Same as in the tokenized version; the sub-parse just operates on
characters instead of tokens.

### Scannerless gotchas: `@longest` and `@keyword`

A traditional parser has a **lexer** stage that pre-tokenises the
input greedily: digits run together into one `NUMBER`, letters into
one `IDENT`, and the exact string `class` resolves to the `CLASS`
keyword *only* when it's a complete word (not a prefix of
`classroom`).  The parser sees tokens, not characters, and inherits
this disambiguation for free.

Our scannerless parser has no lexer — every rule matches characters
directly.  That gives context-sensitive lexing without modes (a real
win), but it also means two things you'd expect to work "naturally"
*don't*, and need directives to fix.

#### `@longest` — replaces lexer-style maximal munch

A rule like `NUMBER = /\d+/` reads "one or more digits."  In an
GPDA parser's all-paths semantics, every intermediate length is a
valid match — on input `"123"`, `NUMBER` matches `"1"`, `"12"`, and
`"123"`, and the parser forks a cursor at each position.  Some of
those cursors die later, but the fork-set is large, and the final
parse tree might pick an unexpected shorter match if the rest of
the grammar happens to accept it too.

Traditional lexers sidestep this because their DFA is greedy by
construction — they commit to the longest match of each token.
`@longest` restores that behaviour for a specific rule:

```
NUMBER = /\d+/ @longest
IDENT  = /[a-zA-Z_]\w*/ @longest
STRING = '"' char* '"' @longest
```

With `@longest`, on input `"123"`, `NUMBER` emits **only** the
longest-match cursor (at position 3).  Single behaviour change.
It's also a significant perf win — fewer cursors per token.

Constraints:
- `@longest` only applies to DFA-compilable rules (regex-like: no
  predicates, no captures, no backrefs, no recursion).  Applying it
  to a non-regular rule is a grammar error.
- The effect is observable at parse time — one cursor per entry,
  not one per accept position.

**When to use it:** on essentially every token-like rule.  The only
reason *not* to mark a token `@longest` is if the grammar genuinely
wants to consider shorter matches (rare — usually a sign of an
ambiguous grammar that needs restructuring).

#### `@keyword` — replaces lexer-level keyword recognition

In a tokenised parser, `'class'` literally written in a grammar
matches only the `CLASS` token — not `classify`, not `classroom`.
The longest-match lexer has already decided `classify` is one
`IDENT`, so `'class'` doesn't match it.

Scannerless: a literal string `'class'` matches the 5 characters
`c`, `l`, `a`, `s`, `s` wherever they appear, including as a prefix
of a longer word.  On input `classroom`:

```
stmt = 'class' IDENT                # naive grammar
```

the parser would explore a cursor that matched `'class'` as the
keyword and then tried to parse `room` as an `IDENT`.  If the
surrounding grammar happens to accept that decomposition, the parse
silently picks the wrong interpretation.

The hand-written fix is a negative lookahead after every keyword
literal:

```
wordchar = [a-zA-Z_0-9]
stmt = 'class' !wordchar IDENT      # 'class' must be a whole word
```

Tedious for many keywords; easy to forget at a call site.  `@keyword`
automates it.  Two spellings, same meaning, different scope:

```
# Top-level: affects every occurrence of the listed strings globally.
@keyword 'class' 'if' 'while' 'for' 'return'

stmt = 'class' IDENT ...            # auto-wrapped: ('class' !wordchar) ...
```

```
# Inline: wraps only this occurrence.
stmt = @keyword 'class' IDENT       # ('class' !wordchar) IDENT
```

Prefer the top-level form when the keyword should apply grammar-wide
— it can't be forgotten at a call site.

**When to use it:** any time you write a literal word that could also
appear as a prefix of an identifier (or any longer word-ish token).
Essentially every keyword in a programming-language-style grammar.

For keyword literals ending with a non-word character (e.g. `'->'`
or `';'`), `@keyword` is a harmless no-op — those literals can't be
prefixes of identifiers anyway.

#### Summary

| Gotcha | Traditional parser | Scannerless parser | Fix |
|---|---|---|---|
| "NUMBER should match longest run of digits" | Lexer DFA is greedy | All valid prefixes produce cursors | `@longest` |
| "'class' shouldn't match classroom" | Lexer longest-match picks IDENT | 5-char literal match succeeds | `@keyword` |

Both directives restore conveniences the tokenisation step would
have given you.  In return, scannerless gives you context-sensitive
lexing, unified grammar syntax, and lexer-mode-free embedded
sub-grammars.  Pick the trade-off deliberately.

### Ambiguity and ordered choice

When a grammar is *ambiguous* (the same input has more than one valid
parse tree), our parser follows every valid path simultaneously and
multiple cursors reach completion.  To pick a single result
deterministically, the parser uses **ordered choice**: among
successful completions, the one whose decisions favoured
earlier-listed alternatives at each `|` fork wins.

```
expr = l:=(atom) '*' r:=(expr)    # tried at priority 0 at this fork
     | l:=(atom) '+' r:=(expr)    # priority 1
     | a:=(atom)                  # priority 2
```

For input `"1+2*3"`, the parser could theoretically form `(1+2)*3` or
`1+(2*3)`.  With the alternatives above, the `'*'` alternative is
priority 0, so the parser picks the parse that takes the `'*'`
alternative at the outer expr.  That gives `1 + (2*3) = 7`.

Ordered choice applies at **every fork in the graph** — grammar-level
`|`, the greedy/non-greedy branch of `*`/`+`/`?`, and the
RULE_END-return when a rule was entered from multiple call sites.
Priority is a tuple of link-indices collected along the parse path;
lexicographic comparison picks the smallest.

What ordered choice *doesn't* do: it can't disambiguate parses that
took the same sequence of alternatives but differ in structure
(e.g., left- vs right-associative groupings of `1+2+3` with the same
`+` alternative chosen twice).  Those require grammar restructuring
— typically left recursion for left-associative or right recursion
for right-associative.

### Semantic actions (`{{ … }}`)

An alternative can end with a semantic action — a Python expression
enclosed in `{{ … }}` — whose return value becomes the rule's
semantic value.  The action sees named captures as local variables,
plus `_start` and `_end` for the rule's own match span.  The
scannerless parser additionally exposes `_text` (the matched
substring); the tokenized parser exposes `_tokens` (the list of
Token objects covering the span).

```
# scannerless
NUMBER = /\d+/ {{ int(_text) }} @longest

pair = a:=(NUMBER) '+' b:=(NUMBER) {{ a + b }}

atom = n:=(NUMBER) {{ n }}
     | '(' e:=(expr) ')' {{ e }}
```

```
# tokenized
start expr
expr = a:=NUMBER '+' b:=NUMBER {{ int(a) + int(b) }}
NUMBER = /\d+/
```

When ``a:=(X)`` captures a rule ``X`` that has an action, the capture
holds the action's return value, not the matched text/tokens.  When
``X`` has no action, the capture holds the child's computed value
(scannerless falls back to matched text; tokenized falls back to the
single child's value if present).  A rule without its own action
automatically inherits the value of a single non-trivial child —
same default as bison's ``$$ = $1``.

The action lives in its own Python ``eval()`` scope.  Captures are
lexically scoped to the rule that declares them — capturing ``l`` in
rule A doesn't leak into rule B when A calls B.

Order of trailing annotations on a rule definition is flexible, but
the action (per-alternative) must come **before** the rule-level
markers (``@longest``, ``@skip``, ``@atomic``, ``@mode NAME``):

```
NUMBER = /\d+/ {{ int(_text) }} @longest   # action, then @longest
```

Actions are a Python-parser feature (both `gpda.py` and
`gpda_scannerless.py`).  The C++ emitters reject grammars containing
actions, since C++ has no embedded Python interpreter.

### Lexer modes (`@mode`)

Different parts of a grammar can have different skip-rule sets active.
Each skip rule and each non-skip rule may be tagged with a mode name
via `@mode NAME`:

```
ws  = /\s+/ @skip                       # default mode (implicit)
esc = /\\./ @skip @mode string          # only active in "string" mode

prog = stmt*                            # default mode (implicit)
str  = '"' char_or_esc* '"' @mode string
```

Inside `prog`, auto-insert uses `ws` between elements.  Inside `str`,
auto-insert uses `esc` instead — so whitespace is significant inside
strings, but backslash-escape sequences are skippable.

Details:
- Untagged rules and skip rules are in the `default` mode.
- A mode with no matching skip rules behaves like `@atomic` (no
  auto-insertion inside).
- Modes are static: each rule has exactly one mode, determined at
  grammar-load time.  No runtime mode stack.
- `_skip_<mode>` names are synthesized per mode and stripped from the
  parse tree output like any other skip reference.

### Atomic rules (`@atomic`)

Rules whose sequence should not have auto-whitespace inserted between
elements can be marked `@atomic`:

```
time = [0-9]+ ':' [0-9]+ ':' [0-9]+ @atomic
```

`time` is a lowercase rule, so ordinarily auto-ws would be inserted
between each atom (turning `"12:34:56"` into `"12 : 34 : 56"`).  With
`@atomic` it stays contiguous.  ALL-CAPS rule names are implicitly
atomic and need no marker; `@atomic` extends the same treatment to
non-uppercase rules.

This is the primary way to express "no whitespace here, even though
the surrounding grammar is ws-flexible" — i.e. context-scoped
skipping.  Anywhere you want to disable auto-skip for a subtree,
name that subtree via a rule and mark it `@atomic`.

### Skip rules (`@skip`) and the `_` rule

Scannerless grammars offer two ways to handle "invisible" content like
whitespace or comments between elements:

**The traditional `_` rule.**  If the grammar defines a rule named `_`,
the parser automatically inserts `_` between consecutive elements in
every non-atomic rule.  The `_` body is expected to already match
"zero-or-more" (e.g. `_ = /\s*/`).

**The `@skip` marker.**  Any rule can be marked `@skip` at the end of
its definition:

```
ws  = /\s+/       @skip
cmt = /#[^\n]*/   @skip
```

The parser auto-inserts a synthetic `(ws | cmt)*` between consecutive
elements in non-atomic rules, so any combination of the skip rules
(including none) is permitted between elements.  Explicit references
to a skip rule are still required where mentioned (same "classic
keyword-plus-whitespace" treatment as in the tokenized parser).

`_` and `@skip` can be combined — `_` is absorbed into the set of skip
rules.

### Auto-whitespace insertion (the `_` rule)

If the grammar defines a rule named `_`, the parser automatically
inserts `_` between consecutive elements in every non-atomic rule:

```
_ = [ \t\n]*
expr = term '+' term              # auto-becomes: term _ '+' _ term
```

**Atomic rules** (no auto-insertion):

1. The `_` rule itself.
2. Rule names that are ALL-UPPERCASE.

```
IDENT = [a-zA-Z_] [a-zA-Z_0-9]*   # atomic: no _ between the two classes
expr  = IDENT '=' IDENT           # non-atomic: _ inserted around '='
```

Insertion is suppressed adjacent to an explicit `_` reference, so
hand-written whitespace markers aren't duplicated.

### Formal EBNF (scannerless) <a id="formal-ebnf-scannerless"></a>

```ebnf
(* Meta-grammar for gpda_scannerless.py *)

grammar      = { comment | newline } { decl } ;
decl         = start_decl | rule ;
start_decl   = "start" , NAME , newline ;
rule         = NAME , "=" , alternatives , [ "@skip" ] , newline ;

alternatives = sequence , { { newline } , "|" , sequence } ;
sequence     = { element } ;
element      = atom , [ modifier ] , [ "-" , atom , [ modifier ] ] ;

atom         = STRING
             | CHARCLASS
             | REGEX
             | "."
             | capture
             | NAME                              (* rule ref/backref *)
             | "(" , alternatives , ")"
             | "&" , atom
             | "!" , atom ;

capture      = NAME , ":=" , ( atom | "(" , alternatives , ")" ) ;

modifier     = "*" | "*?" | "+" | "+?" | "?" | "??"
             | "{" , NUMBER , [ "," , NUMBER ] , "}" , [ "?" ] ;

(* Lexical tokens *)
STRING       = "'" , { escape | any_char - ("'" | "\") } , "'"
             | '"' , { escape | any_char - ('"' | "\") } , '"' ;
CHARCLASS    = "[" , [ "^" ] , { class_item } , "]" ;
class_item   = escape
             | char , [ "-" , char ] ;            (* char or range *)
escape       = "\" , any_char ;
NAME         = ("a".."z" | "A".."Z" | "_") , { ("a".."z" | "A".."Z" | "0".."9" | "_") } ;
NUMBER       = "0".."9" , { "0".."9" } ;
comment      = "#" , { any_char - newline } ;
```

### Auto-`_` rule rationale <a id="auto-_-rule-rationale"></a>

#### The problem

In a scannerless parser every character is significant — there's no
lexer stage to silently eat whitespace. So the naive grammar

```
expr = term '+' term
term = [0-9]+
```

parses `"1+2"` but rejects `"1 + 2"`: the space has nowhere to go.
To fix it, the author has to thread a whitespace rule through every
sequence in every rule:

```
ws = [ \t\n]*
expr = ws term ws '+' ws term ws
```

This is tedious, error-prone, and easy to forget in one spot,
producing silent bugs.

#### The solution

Defining a rule named `_` *opts in* to auto-insertion. The preprocessor
walks the grammar and inserts `_` between consecutive elements in
every non-atomic rule. The author writes the natural form:

```
_ = [ \t\n]*
expr = term '+' term
```

and the preprocessor transforms it (conceptually) to:

```
_ = [ \t\n]*
expr = term _ '+' _ term
```

#### Why atomic rules exist

Not every rule tolerates whitespace between its elements. An
identifier, for example, must be a contiguous run of characters:

```
IDENT = [a-zA-Z_] [a-zA-Z_0-9]*
```

If `_` were inserted here, `"foo bar"` could be parsed as a single
identifier — clearly wrong. Hence the atomic-rule convention:

- `_` itself — to avoid infinite recursion in the auto-insertion pass.
- UPPERCASE rule names — a familiar convention (users of most parser
  generators already associate uppercase with "token-like atomic
  things").

#### Why explicit `_` suppresses adjacent insertion

A user may already have handled whitespace in some specific spot:

```
stmt = 'return' _ value
```

Without suppression, auto-insertion would turn this into
`'return' _ _ _ value`. Three adjacent `_` references matching zero or
more characters still works, but it's wasteful and confusing in
diagnostic output. The preprocessor therefore skips insertion whenever
the previous or next element is already a bare `_` reference.

#### Boundary whitespace

Auto-insertion is only *between* elements, never at the start or end
of a rule body. Inserting at boundaries would cause every composed
rule reference to double up on whitespace in diagnostic output (every
`A B` would expand to `_ A _ _ B _`). The clean convention instead is
to wrap the start rule manually if leading/trailing whitespace is
needed:

```
prog = _ expr _
```

---

## Common operators

| Operator  | Meaning                                    |
|-----------|--------------------------------------------|
| `|`       | Alternation                                |
| `*`       | Zero-or-more (greedy)                      |
| `*?`      | Zero-or-more (non-greedy)                  |
| `+`       | One-or-more (greedy)                       |
| `+?`      | One-or-more (non-greedy)                   |
| `?`       | Optional (greedy — try match first)        |
| `??`      | Optional (non-greedy — try skip first)     |
| `{n}`     | Exact repetition                           |
| `{n,m}`   | Bounded repetition (greedy)                |
| `{n,m}?`  | Bounded repetition (non-greedy)            |
| `( ... )` | Grouping                                   |
| `&`       | AND predicate (zero-width lookahead)       |
| `!`       | NOT predicate (zero-width negative lookahead) |
| `:=`      | Named capture                              |
| `A - B`   | Subtraction: match `A`, reject if `B` matches the same span |

Greedy/non-greedy only affects the *priority order* of link traversal
in the graph. With zero-lookahead parsing, both variants still find
the same complete parses; the greedy variant prefers longer matches
when multiple parses are valid.

---

## Operator precedence (`@left` / `@right` / `@nonassoc`)

Top-level **precedence declarations** rewrite ambiguous binary-operator
rules into an unambiguous precedence ladder at grammar-load time. They
let you write the natural ambiguous form

```
expr = expr '+' expr
     | expr '*' expr
     | NUMBER
```

instead of the explicit hierarchy

```
expr = term ('+' term)*
term = factor ('*' factor)*
factor = NUMBER
```

…and still get linear-time parsing.

Syntax:

```
@left      'op1' 'op2' ...      # left-associative, this precedence level
@right     'op1' 'op2' ...      # right-associative
@nonassoc  'op1' 'op2' ...      # non-associative (a OP b OP c is rejected)
```

Operators may be **string literals** (`'+'`) or **bare names**:

```
@left  PLUS MINUS               # token-type refs (tokenized parser)
@left  plus times               # rule refs (scannerless parser)
@left  '+' PLUS '*'             # mixed is fine
```

A bare-name operator matches alts whose middle element is a token
reference (tokenized) or a rule reference (scannerless). String
literals match alts whose middle element is a string literal.

Earlier declarations bind **looser** (lower precedence), matching
bison/yacc convention. Operators on the same `@left`/`@right`/`@nonassoc`
line share precedence and associativity.

Example with full precedence:

```
@left   '+' '-'
@left   '*' '/' '%'
@right  '^'                    # right-assoc: 2^3^4 = 2^(3^4)
@nonassoc '==' '<' '>'         # 1 == 2 == 3 → parse error

expr = expr '+' expr | expr '-' expr
     | expr '*' expr | expr '/' expr | expr '%' expr
     | expr '^' expr
     | expr '==' expr | expr '<' expr | expr '>' expr
     | NUMBER | '(' expr ')'
```

### What the rewrite does

For each rule whose alternatives include patterns of shape
`R OP R` (self-recursive on both sides with a string-literal operator
that appears in the precedence table), the loader generates a ladder:

- `R` (the original name) becomes the lowest-precedence rule.
- `_R_p1`, `_R_p2`, … are intermediate rules, one per precedence
  level above the lowest.
- `_R_atom` collects the non-binary alternatives (`NUMBER`,
  `'(' expr ')'` etc.).

Each level is generated as left/right/non recursive depending on the
associativity declared for its operators; left-recursion is then
handled by the existing LR-elimination pass, so the parse tree comes
out left-associative when expected.

References to `R` from elsewhere in the grammar (including the
`'(' R ')'` alt inside `_R_atom`) point to the rule's *outermost*
level, so the precedence ladder wraps around for parenthesized
sub-expressions.

### Limitations

- **Pattern-bound**: only triggers on alternatives of exactly shape
  `[ref-to-R, op, ref-to-R]` where `op` is a string literal, a
  token reference (tokenized), or a rule reference (scannerless),
  and is declared in the precedence table. Captures, modifiers, or
  multi-element operands disqualify that alternative.
- **Atom required**: at least one non-binary alternative must exist
  (the recursion needs a base case).
- **No actions on binary alts**: actions on a binary alternative
  (`expr '+' expr {{ ... }}`) aren't preserved by the rewrite. Put
  actions on the atom alternatives, or hand-write the ladder.
- **One operator per binary alt**: an alt like `expr '<' '=' expr`
  (a single rule body matching two literals) doesn't fit the pattern;
  declare `'<='` as one token instead.

---

## Feature matrix

| Feature                           | `gpda.py`       | `gpda_scannerless.py` |
|-----------------------------------|-----------------|-----------------------|
| Grammar rules + alternatives      | ✓               | ✓                     |
| Sequence, `*`, `+`, `?`, `{n,m}`  | ✓               | ✓                     |
| Non-greedy modifiers (`*?` etc.)  | ✓               | ✓                     |
| `&` / `!` predicates              | ✓               | ✓                     |
| `A - B` subtraction               | ✓               | ✓                     |
| Semantic actions (`{{ … }}`)      | ✓               | ✓                     |
| Named captures (`name:=(expr)`)   | ✓               | ✓                     |
| Backreferences (bare name reuse)  | ✗               | ✓                     |
| Direct + mutual left recursion    | ✓               | ✓                     |
| Literal strings (`'text'`)        | by token value  | by character sequence |
| Token references (`UPPERCASE`)    | by token type   | atomic rule marker    |
| Character classes (`[a-z]`)       | ✗               | ✓                     |
| Shorthand classes (`\d \w \s`)    | ✓ (inside regex)| ✓ (regex + charclass) |
| Any character (`.`)               | ✓ (regex)       | ✓                     |
| Inline regex (`/regex/flags`)     | ✓ (auto-tokens) | ✓ (char-level graph)  |
| Auto-generated lexer              | ✓               | n/a (no lexer)        |
| `@skip` rule marker               | ✓               | ✓                     |
| Auto-`_` whitespace insertion     | ✗               | ✓                     |
| Multiple `@skip` rules combined   | ✓               | ✓                     |
| `@left` / `@right` / `@nonassoc`  | ✓               | ✓                     |
| ISO 14977 EBNF mode (`ebnf=True`) | ✓               | ✓                     |

Both parsers share the same core algorithm (graph-walking with
simultaneous path tracking and zero lookahead). The syntactic
differences reflect the different abstraction level — tokens vs.
characters — at which they operate.

---

## ISO 14977 EBNF mode

Both parsers' `load_grammar` function takes an `ebnf=False` keyword
argument.  Passing `ebnf=True` switches the bootstrap to ISO 14977
EBNF syntax — a strict subset used in formal language specifications:

```python
load_grammar(source, ebnf=True)
```

Supported constructs:

| Syntax                     | Meaning                                |
|----------------------------|----------------------------------------|
| `rule = expression ;`      | Rule definition (semicolon-terminated) |
| `A , B`                    | Concatenation (explicit comma)         |
| `A | B`                    | Alternation                            |
| `[ A ]`                    | Optional (zero or one)                 |
| `{ A }`                    | Zero-or-more                           |
| `( A )`                    | Grouping                               |
| `N * A`                    | Exactly *N* copies of `A`              |
| `A - B`                    | Subtraction (same as in EEBNF mode)    |
| `"text"`, `'text'`         | Terminal string                        |
| `(* comment *)`            | Comment                                |

Rejected with a parse error:
- `? special sequence ?` — implementation-defined, cannot be
  interpreted.
- Any EEBNF extension (no `/regex/`, no `@skip`, no `@keyword`, no
  captures, no predicates, no actions).

In **scannerless** mode, terminal strings match contiguous characters
— the grammar is genuinely equivalent to standard EBNF.

In **tokenized** mode, ISO EBNF has no concept of a lexer, so the
caller supplies pre-lexed tokens directly to `GraphParser.parse(...)`
(bring-your-own-lexer).  Terminal strings match a token's `value`
field.  `load_grammar(text, ebnf=True)` returns a `GraphParser`
directly (no auto-lexer wrapper).

The C++ emitter scripts accept the same flag on the command line:

```
python cpp/benchmarks/scripts/emit_cpp_graph.py       --ebnf  grammar.ebnf  out.cpp  fn_name
python cpp/benchmarks/scripts/emit_tokenized_graph.py --ebnf  grammar.ebnf  out.cpp  fn_name
```
