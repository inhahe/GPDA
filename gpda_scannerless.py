"""
gpda_scannerless.py - Scannerless GPDA Graph Parser
=====================================================
Character-by-character version of the GPDA parser.  No separate lexer
stage — the grammar describes both lexical and syntactic structure in a
single unified notation and the parser reads raw characters.

Terminals:
    'text'    literal string (expanded to a chain of character matches)
    "text"    same (both quote styles)
    [a-z]     character class (regex-style ranges)
    [^0-9]    negated character class
    .         any single character
    \\n \\t   escape sequences (in strings and character classes)

Rules:
    name = ...          (all names are rules; no token/rule distinction)

Operators: same as gpda.py
    |  *  *?  +  +?  ?  ??  {n}  {n,m}  ()
    &       AND predicate (zero-width lookahead)
    !       NOT predicate (zero-width negative lookahead)

Named captures and backreferences:
    name:=(expr)   capture the text matched by expr under 'name'
    name           within the same rule, a bare reference to a capture
                   name is a backreference — it matches the exact text
                   that was captured earlier (captures shadow rules).

    Example — matching paired quotes:
        string = q:=(['"]) (!q .)* q

    Example — here-documents:
        heredoc = '<<' tag:=([a-zA-Z]+) '\\n' (!('\\n' tag) .)* '\\n' tag

Auto-whitespace insertion:
    If the grammar defines a rule named `_`, it is automatically inserted
    between consecutive elements in sequences of non-atomic rules:

        _ = [ \\t\\n]*
        expr = term '+' term        # auto-becomes: term _ '+' _ term

    Atomic rules (no auto-insertion):
        - the `_` rule itself
        - UPPERCASE rule names (convention for token-like atomic rules)

    Leading/trailing whitespace is NOT auto-handled; wrap the start rule
    explicitly:   `prog = _ expr _`

Parse tree:
    Each rule match produces a ParseNode whose `text` attribute holds the
    raw matched substring.  Children are sub-rule matches only — character
    matches are implicit in `text`.

Based on: https://exalumen.blog/2025/03/06/epeg-a-new-way-of-parsing/
"""

import re
import copy


# ========================== Character Classes ==========================

class CharClass:
    """A set of characters defined by ranges and/or individual chars."""
    __slots__ = ('ranges', 'chars', 'negated')

    def __init__(self, ranges=None, chars=None, negated=False):
        self.ranges = ranges or []   # [(lo, hi), ...]
        self.chars = chars or set()  # {ch, ...}
        self.negated = negated

    def matches(self, ch):
        hit = ch in self.chars or any(lo <= ch <= hi for lo, hi in self.ranges)
        return not hit if self.negated else hit

    def __repr__(self):
        neg = '^' if self.negated else ''
        parts = [f'{lo}-{hi}' for lo, hi in self.ranges]
        parts += sorted(self.chars)
        return f'[{neg}{"".join(parts)}]'


def _parse_charclass(text):
    """Parse the interior of ``[...]`` into a CharClass.  Recognises
    shorthand escapes (\\d \\w \\s and their negations) and standard
    escape sequences (\\n \\t \\r \\f \\v \\0)."""
    i = 0
    negated = False
    if i < len(text) and text[i] == '^':
        negated = True
        i += 1
    cc = CharClass(negated=negated)
    while i < len(text):
        # Shorthand class merged directly into the enclosing class.
        if text[i] == '\\' and i + 1 < len(text) \
                and text[i + 1] in _SHORTHAND:
            inner = _SHORTHAND[text[i + 1]]()
            if inner.negated:
                raise SyntaxError(
                    f"negated shorthand '\\{text[i + 1]}' cannot be "
                    f"nested inside [...]")
            cc.ranges.extend(inner.ranges)
            cc.chars.update(inner.chars)
            i += 2
            continue
        c, i = _read_escape(text, i)
        if i < len(text) and text[i] == '-' and i + 1 < len(text):
            i += 1
            if text[i] == '\\' and i + 1 < len(text) \
                    and text[i + 1] in _SHORTHAND:
                raise SyntaxError(
                    'range end cannot be a shorthand character class')
            c2, i = _read_escape(text, i)
            cc.ranges.append((c, c2))
        else:
            cc.chars.add(c)
    return cc


def _read_escape(text, i):
    """Read one (possibly escaped) character.  Returns (char, new_i)."""
    if text[i] == '\\' and i + 1 < len(text):
        c = text[i + 1]
        return ({'n': '\n', 't': '\t', 'r': '\r',
                 'f': '\f', 'v': '\v', '0': '\0'}.get(c, c), i + 2)
    return text[i], i + 1


def _unescape(s):
    """Process escape sequences in a quoted string literal."""
    out = []
    i = 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            c = s[i + 1]
            out.append({'n': '\n', 't': '\t', 'r': '\r',
                        'f': '\f', 'v': '\v', '0': '\0'}.get(c, c))
            i += 2
        else:
            out.append(s[i])
            i += 1
    return ''.join(out)


# ========================== Inline Regex → Atom AST ==========================

def _split_regex(raw):
    """Given a raw '/pattern/flags' token, return (pattern, flags).

    The pattern may contain escaped slashes; the first unescaped `/` at
    index 0 opens and the last unescaped `/` closes."""
    assert raw.startswith('/')
    # Scan from left to find the closing slash, respecting \-escapes.
    i = 1
    while i < len(raw):
        if raw[i] == '\\' and i + 1 < len(raw):
            i += 2
            continue
        if raw[i] == '/':
            return raw[1:i], raw[i + 1:]
        i += 1
    raise SyntaxError(f'unterminated regex literal: {raw}')


# Shorthand classes.  These produce CharClass values directly so they can
# be composed inside [...] too (e.g. [a-z\d_]).
_SHORTHAND = {
    'd': lambda: CharClass(ranges=[('0', '9')]),
    'D': lambda: CharClass(ranges=[('0', '9')], negated=True),
    'w': lambda: CharClass(ranges=[('a', 'z'), ('A', 'Z'), ('0', '9')],
                           chars={'_'}),
    'W': lambda: CharClass(ranges=[('a', 'z'), ('A', 'Z'), ('0', '9')],
                           chars={'_'}, negated=True),
    's': lambda: CharClass(chars={' ', '\t', '\n', '\r', '\f', '\v'}),
    'S': lambda: CharClass(chars={' ', '\t', '\n', '\r', '\f', '\v'},
                           negated=True),
}

_REGEX_SIMPLE_ESCAPE = {
    't': '\t', 'n': '\n', 'r': '\r', 'f': '\f', 'v': '\v', '0': '\0',
}


class _RegexParser:
    """Compile a regex pattern string into an atom-AST element compatible
    with the rest of the grammar loader.  Recognised features:
        . literal char
        \\t \\n \\r \\f \\v \\0 \\\\ \\/ and any \\X (X becomes itself)
        \\xHH  \\uHHHH
        [abc] [^abc] [a-z] (with shorthands \\d \\w \\s inside)
        \\d \\D \\w \\W \\s \\S shorthand classes
        ( ... )          capturing group (treated as non-capturing here;
                         named captures come from name:=(...) at the
                         grammar level, not inside regex)
        (?: ... )        non-capturing group (same semantics)
        |                alternation
        * + ? {n} {n,} {n,m}  quantifiers (with trailing ? = non-greedy)
    Not supported: anchors (^ $ \\b), lookarounds (use & ! at grammar
    level), named groups, backrefs inside the regex."""

    def __init__(self, text, case_insensitive=False, unicode=False):
        self.text = text
        self.pos = 0
        self.ci = case_insensitive
        self.u = unicode
        self._in_class = False  # True while parsing contents of a [...]

    def error(self, msg):
        raise SyntaxError(f'regex: {msg} at pos {self.pos} in /{self.text}/')

    def _peek(self, off=0):
        i = self.pos + off
        return self.text[i] if i < len(self.text) else ''

    def _adv(self):
        self.pos += 1

    def compile(self):
        alts = self._parse_alternatives()
        if self._peek():
            self.error(f'unexpected {self._peek()!r}')
        if len(alts) == 1 and len(alts[0]) == 1:
            return alts[0][0]
        return {'type': 'group', 'alternatives': alts}

    # ---- grammar productions ----

    def _parse_alternatives(self):
        alts = [self._parse_sequence()]
        while self._peek() == '|':
            self._adv()
            alts.append(self._parse_sequence())
        return alts

    def _parse_sequence(self):
        seq = []
        while self._peek() and self._peek() not in ')|':
            seq.append(self._parse_term())
        return seq

    def _parse_term(self):
        atom = self._parse_atom()
        # Trailing quantifier?
        c = self._peek()
        if c in '*+?':
            self._adv()
            op = c
            if self._peek() == '?':
                self._adv()
                op += '?'
            atom['modifier'] = op
        elif c == '{':
            self._adv()
            # Accept {,m} (min=0 implicit) as well as {n}, {n,}, {n,m}.
            if self._peek() == ',':
                self._adv()
                lo = 0
                hi = self._read_int()
            else:
                lo = self._read_int()
                hi = lo
                if self._peek() == ',':
                    self._adv()
                    hi = self._read_int(allow_empty=True)
                    # empty hi means unbounded
            if self._peek() != '}':
                self.error('expected }')
            self._adv()
            greedy = True
            if self._peek() == '?':
                self._adv()
                greedy = False
            atom['modifier'] = {'min': lo,
                                'max': hi if hi is not None else -1,
                                'greedy': greedy}
        return atom

    def _read_int(self, allow_empty=False):
        s = ''
        while self._peek().isdigit():
            s += self._peek()
            self._adv()
        if not s:
            if allow_empty:
                return None
            self.error('expected number')
        return int(s)

    def _parse_atom(self):
        c = self._peek()
        if c == '(':
            self._adv()
            # Optional non-capturing marker (?:
            if self._peek() == '?' and self._peek(1) == ':':
                self._adv(); self._adv()
            alts = self._parse_alternatives()
            if self._peek() != ')':
                self.error('expected )')
            self._adv()
            return {'type': 'group', 'alternatives': alts}
        if c == '[':
            return self._parse_class()
        if c == '.':
            self._adv()
            if self.u:
                # Match one full UTF-8 code point (1-4 bytes).
                return self._utf8_codepoint_atom()
            return {'type': 'any'}
        if c == '\\':
            return self._parse_escape_atom()
        # Single literal character
        self._adv()
        return self._char_atom(c)

    def _char_atom(self, c):
        """Produce either a string atom or (for case-insensitive letters)
        a charclass atom matching both cases.  ``c`` may be a multi-char
        string when ``u`` mode expanded a code point into its UTF-8
        byte sequence — in that case case-insensitivity (which is
        ASCII-only) doesn't apply."""
        if self.ci and len(c) == 1 and c.isalpha():
            return {'type': 'charclass',
                    'charclass': CharClass(chars={c.lower(), c.upper()})}
        return {'type': 'string', 'value': c}

    def _utf8_codepoint_atom(self):
        """Under `u` flag, build an atom matching one full UTF-8 code
        point — any of:
            1-byte:  [\\x00-\\x7f]
            2-byte:  [\\xc0-\\xdf][\\x80-\\xbf]
            3-byte:  [\\xe0-\\xef][\\x80-\\xbf]{2}
            4-byte:  [\\xf0-\\xf7][\\x80-\\xbf]{3}
        """
        def cls(lo, hi):
            return {'type': 'charclass',
                    'charclass': CharClass(ranges=[(chr(lo), chr(hi))])}
        cont = (0x80, 0xbf)
        alts = [
            [cls(0x00, 0x7f)],
            [cls(0xc0, 0xdf), cls(*cont)],
            [cls(0xe0, 0xef), cls(*cont), cls(*cont)],
            [cls(0xf0, 0xf7), cls(*cont), cls(*cont), cls(*cont)],
        ]
        return {'type': 'group', 'alternatives': alts}

    def _parse_escape_atom(self):
        assert self._peek() == '\\'
        self._adv()
        esc = self._peek()
        if not esc:
            self.error('dangling backslash')
        self._adv()
        if esc in _SHORTHAND:
            return {'type': 'charclass', 'charclass': _SHORTHAND[esc]()}
        lit = self._decode_literal_escape(esc)
        return self._char_atom(lit)

    def _decode_literal_escape(self, esc):
        """Resolve a backslash escape that represents a single literal
        character (not a shorthand class)."""
        if esc in _REGEX_SIMPLE_ESCAPE:
            return _REGEX_SIMPLE_ESCAPE[esc]
        if esc == 'x':
            h = self._peek() + self._peek(1)
            if len(h) < 2 or not all(c in '0123456789abcdefABCDEF' for c in h):
                self.error('expected two hex digits after \\x')
            self._adv(); self._adv()
            return chr(int(h, 16))
        if esc == 'u':
            h = ''.join(self._peek(k) for k in range(4))
            if len(h) < 4 or not all(c in '0123456789abcdefABCDEF' for c in h):
                self.error('expected four hex digits after \\u')
            for _ in range(4):
                self._adv()
            cp = int(h, 16)
            # Under `u` flag, encode non-ASCII code points to their UTF-8
            # byte sequence (returned as a latin-1-decoded str so each
            # char in the result corresponds to one UTF-8 byte).  Skip
            # this expansion when decoding inside a [...] class — the
            # class builder needs the original code point to decide how
            # to split ranges into byte patterns.
            if self.u and cp > 0x7f and not self._in_class:
                return chr(cp).encode('utf-8').decode('latin-1')
            return chr(cp)
        # Literal escape — the escaped char is itself (covers \\ \/ \. \+ \( etc.)
        return esc

    def _parse_class(self):
        """Parse a [...] regex char class into a charclass atom.
        Supports \\d \\w \\s shortcuts inside the brackets.

        Under `u` flag, classes containing non-ASCII code points are
        expanded into an alternation of UTF-8 byte-pattern sequences
        (see `_utf8_expand_class`).  Negated classes with non-ASCII
        content aren't supported — complementing a codepoint set in
        UTF-8 byte space is non-trivial."""
        assert self._peek() == '['
        self._adv()
        negated = False
        if self._peek() == '^':
            negated = True
            self._adv()
        cc = CharClass(negated=negated)
        self._in_class = True
        try:
            while self._peek() and self._peek() != ']':
                self._add_class_item(cc)
        finally:
            self._in_class = False
        if self._peek() != ']':
            self.error('expected ]')
        self._adv()

        if self.u and _class_has_non_ascii(cc):
            if negated:
                self.error(
                    'negated char class with non-ASCII code points is '
                    'not supported under /u flag (encode the complement '
                    'explicitly as an alternation)')
            return _utf8_expand_class(cc)
        return {'type': 'charclass', 'charclass': cc}

    def _add_class_item(self, cc):
        """Merge one [...] item into ``cc``: either a shorthand class, a
        single char, or a range."""
        start = self._read_class_char(allow_shorthand_merge=cc)
        if start is None:
            # Already merged a shorthand class into cc; no range follows.
            return
        # Optional range
        if self._peek() == '-' and self._peek(1) and self._peek(1) != ']':
            self._adv()
            end = self._read_class_char(allow_shorthand_merge=None)
            if end is None:
                self.error('range end cannot be a shorthand class')
            cc.ranges.append((start, end))
            if self.ci and start.isalpha() and end.isalpha():
                cc.ranges.append((start.lower(), end.lower()))
                cc.ranges.append((start.upper(), end.upper()))
        else:
            cc.chars.add(start)
            if self.ci and start.isalpha():
                cc.chars.add(start.lower())
                cc.chars.add(start.upper())

    def _read_class_char(self, allow_shorthand_merge):
        """Read one character (possibly escape) from inside [...].
        Returns the literal char, or None if a shorthand class was merged
        into ``allow_shorthand_merge`` (which must be the enclosing cc)."""
        c = self._peek()
        if c == '\\':
            self._adv()
            esc = self._peek()
            self._adv()
            if esc in _SHORTHAND:
                if allow_shorthand_merge is None:
                    return None
                inner = _SHORTHAND[esc]()
                if inner.negated:
                    raise SyntaxError(
                        f'negated shorthand \\{esc} not allowed as a range '
                        f'endpoint or inside another class')
                allow_shorthand_merge.ranges.extend(inner.ranges)
                allow_shorthand_merge.chars.update(inner.chars)
                return None
            return self._decode_literal_escape(esc)
        self._adv()
        return c


def _utf8_encode_cp(cp):
    """Return the UTF-8 byte list for a code point."""
    if cp <= 0x7F:
        return [cp]
    if cp <= 0x7FF:
        return [0xC0 | (cp >> 6), 0x80 | (cp & 0x3F)]
    if cp <= 0xFFFF:
        return [0xE0 | (cp >> 12), 0x80 | ((cp >> 6) & 0x3F),
                0x80 | (cp & 0x3F)]
    return [0xF0 | (cp >> 18), 0x80 | ((cp >> 12) & 0x3F),
            0x80 | ((cp >> 6) & 0x3F), 0x80 | (cp & 0x3F)]


def _utf8_byte_ranges(cp_lo, cp_hi):
    """Given a code-point range [cp_lo, cp_hi], return a list of
    byte-pattern alternatives.  Each alternative is a list of
    ``(byte_lo, byte_hi)`` tuples — one per byte position — describing
    a contiguous run of UTF-8 byte sequences that maps exactly to a
    sub-range of the input code-point range.

    Collectively the alternatives exactly cover the input range with
    no overlaps.  Implements the standard "UTF-8 range split"
    decomposition (used by re2 / Rust's regex-automata etc.)."""
    if cp_lo > cp_hi:
        return []
    out = []
    tier_bounds = [0x7F, 0x7FF, 0xFFFF, 0x10FFFF]
    lo = cp_lo
    for boundary in tier_bounds:
        if lo > boundary:
            continue
        sub_hi = min(cp_hi, boundary)
        out.extend(_decompose_same_tier(_utf8_encode_cp(lo),
                                         _utf8_encode_cp(sub_hi)))
        lo = sub_hi + 1
        if lo > cp_hi:
            break
    return out


def _decompose_same_tier(lo_bytes, hi_bytes):
    """Decompose a same-length byte range into a list of byte-range
    alternatives.  Each alternative is a list of ``(byte_lo, byte_hi)``
    describing a contiguous run of byte sequences of this length."""
    assert len(lo_bytes) == len(hi_bytes)
    n = len(lo_bytes)
    if n == 1:
        return [[(lo_bytes[0], hi_bytes[0])]]
    if lo_bytes[0] == hi_bytes[0]:
        sub = _decompose_same_tier(lo_bytes[1:], hi_bytes[1:])
        return [[(lo_bytes[0], lo_bytes[0])] + r for r in sub]
    out = []
    # Low-edge: fixed first byte = lo_bytes[0], tail goes from lo_bytes[1:]
    # up to [0xBF]*(n-1).
    sub = _decompose_same_tier(lo_bytes[1:], [0xBF] * (n - 1))
    out.extend([[(lo_bytes[0], lo_bytes[0])] + r for r in sub])
    # Middle: first byte in (lo_bytes[0]+1, hi_bytes[0]-1), all tails
    # spanning full continuation range.
    if hi_bytes[0] > lo_bytes[0] + 1:
        out.append([(lo_bytes[0] + 1, hi_bytes[0] - 1)]
                   + [(0x80, 0xBF)] * (n - 1))
    # High-edge: fixed first byte = hi_bytes[0], tail from [0x80]*(n-1)
    # to hi_bytes[1:].
    sub = _decompose_same_tier([0x80] * (n - 1), hi_bytes[1:])
    out.extend([[(hi_bytes[0], hi_bytes[0])] + r for r in sub])
    return out


def _class_has_non_ascii(cc):
    """True if any char or range endpoint in ``cc`` is above 0x7F."""
    for c in cc.chars:
        if ord(c) > 0x7F:
            return True
    for lo, hi in cc.ranges:
        if ord(lo) > 0x7F or ord(hi) > 0x7F:
            return True
    return False


def _utf8_expand_class(cc):
    """Convert a CharClass with non-ASCII content into a group atom
    whose alternatives are UTF-8 byte-pattern sequences.  Each
    single-char and range contributes one or more alternatives.  ASCII
    items stay as single-byte alternatives for consistency."""
    alternatives = []
    for c in cc.chars:
        cp = ord(c)
        for pattern in _utf8_byte_ranges(cp, cp):
            alternatives.append(_byte_pattern_to_atoms(pattern))
    for lo, hi in cc.ranges:
        for pattern in _utf8_byte_ranges(ord(lo), ord(hi)):
            alternatives.append(_byte_pattern_to_atoms(pattern))
    # If exactly one single-byte alt remains, return a plain CharClass
    # instead of a group wrapper (micro-optimisation for single-char
    # non-ASCII classes).
    return {'type': 'group', 'alternatives': alternatives}


def _byte_pattern_to_atoms(byte_ranges):
    """Turn ``[(byte_lo, byte_hi), ...]`` into a list of atoms — one per
    byte position — each either a ``string`` (single byte) or
    ``charclass`` (byte range)."""
    atoms = []
    for lo, hi in byte_ranges:
        if lo == hi:
            atoms.append({'type': 'string', 'value': chr(lo)})
        else:
            atoms.append({'type': 'charclass',
                          'charclass': CharClass(ranges=[(chr(lo), chr(hi))])})
    return atoms


def _compile_regex(pattern, flags):
    """Compile a ``/regex/flags`` pattern into a single atom-AST element.

    Supported flags:
        i  — case-insensitive (letters in the pattern match either case)
        x  — verbose: whitespace and ``# comments`` stripped from pattern
             before parsing.  Preserve whitespace/`#` by escaping with ``\\``.
        u  — Unicode: ``\\uHHHH`` and ``.`` emit UTF-8 byte sequences.
             Byte-level; the C++ parser matches UTF-8 input natively.
             In Python (which parses Python str = code points), the `u`
             flag is only useful if the input is bytes-as-latin1 str.
    """
    if 'x' in flags:
        pattern = _strip_verbose(pattern)
    return _RegexParser(pattern,
                        case_insensitive='i' in flags,
                        unicode='u' in flags).compile()


def _strip_verbose(pattern):
    """Remove unescaped whitespace and ``# ... \\n`` comments from a
    regex pattern (the `x` flag).  Inside ``[...]`` brackets nothing is
    stripped — class contents are kept verbatim.  Escaped whitespace
    (``\\ ``) and escaped ``\\#`` are preserved as literal matches."""
    out = []
    i = 0
    in_class = False
    while i < len(pattern):
        c = pattern[i]
        if c == '\\' and i + 1 < len(pattern):
            # Preserve escaped char, including escaped whitespace/#
            out.append(pattern[i])
            out.append(pattern[i + 1])
            i += 2
            continue
        if in_class:
            out.append(c)
            if c == ']':
                in_class = False
            i += 1
            continue
        if c == '[':
            in_class = True
            out.append(c)
            i += 1
            continue
        if c in ' \t\r\n':
            i += 1
            continue
        if c == '#':
            # skip to end of line
            while i < len(pattern) and pattern[i] != '\n':
                i += 1
            continue
        out.append(c)
        i += 1
    return ''.join(out)


# ========================== Semantic Actions ==========================

# The globals dict passed to ``eval()`` when running a ``{{ … }}`` action.
# Users' capture names live in the locals dict; here we expose nothing
# unusual, just a curated __builtins__ so actions can call ``int``, ``len``,
# ``str``, etc.  Tune as the grammar ecosystem demands.
_ACTION_GLOBALS = {'__builtins__': __builtins__}


# ========================== Keyword (@keyword) preprocessor =======================

# The default "word character" class used for keyword boundary checks.
_WORDCHAR = CharClass(ranges=[('a', 'z'), ('A', 'Z'), ('0', '9')],
                      chars={'_'})


def _is_word_char(c):
    return c.isalnum() or c == '_'


def _wrap_keyword(s):
    """Produce the atom AST for a keyword-checked literal string.

    The wrapped form is  ``('<s>' !<wordchar>)`` as a group, ensuring
    the literal can only match when it is not the prefix of a longer
    word.  The group carries an ``atomic=True`` flag so auto-ws
    insertion leaves its inner sequence alone.  If the literal's last
    character is not a word character (e.g. an operator like ``'->'``),
    no wrapping is needed — the @keyword mark becomes a no-op."""
    if not s or not _is_word_char(s[-1]):
        return {'type': 'string', 'value': s}
    not_word = {'type': 'not',
                'alternatives': [[{'type': 'charclass',
                                   'charclass': _WORDCHAR}]]}
    return {'type': 'group',
            'atomic': True,
            'alternatives': [[{'type': 'string', 'value': s}, not_word]]}


def _apply_precedence(rules, precedence):
    """Rewrite rules matching the precedence-ladder pattern.

    For each rule ``R`` whose alternatives include binary patterns of
    shape ``[ref_to_R, str_op, ref_to_R]`` where ``str_op`` is declared
    in ``precedence``, generate a ladder of rules — one per precedence
    level — so the grammar becomes unambiguous.  The parser still uses
    the same "explore all paths, dedup" algorithm; the rewrite just
    gives it a shape where dedup can collapse cursors linearly.

    Associativity:
        left:     ``R_k = R_k OPS R_{k+1} | R_{k+1}``  (LR elim fixes this)
        right:    ``R_k = R_{k+1} OPS R_k | R_{k+1}``
        nonassoc: ``R_k = R_{k+1} (OPS R_{k+1})?``

    Atom alternatives (anything not matching the binary pattern) become
    the innermost rule ``_<R>_atom``.  References to ``R`` from
    anywhere (including ``'(' R ')'`` inside atoms) still resolve to
    the outermost level, so the precedence wraps around correctly.
    """
    if not precedence:
        return rules

    # op_info key is (kind, value):
    #   ('string', '+')          for `@left '+'`
    #   ('name',   'PLUS')       for `@left PLUS` (a rule or token ref)
    op_info = {}
    for level_index, (assoc, ops) in enumerate(precedence):
        for op_kind, op_value in ops:
            key = (op_kind, op_value)
            if key in op_info:
                raise SyntaxError(
                    f"operator {op_value!r} listed in multiple "
                    f"precedence declarations")
            op_info[key] = (level_index, assoc)

    def op_key(node):
        """Return the op_info key for an alt's middle element, or None
        if the element doesn't look like a precedence-table operator."""
        t = node.get('type')
        v = node.get('value')
        if t == 'string':
            return ('string', v)
        # Rule / token references look up under the 'name' kind.  The
        # underlying type is preserved in the rewritten op_node so the
        # tokenized parser keeps token-typed nodes and the scannerless
        # one keeps rule-typed nodes.
        if t in ('rule', 'token'):
            return ('name', v)
        return None

    out = {}
    for name, alts in rules.items():
        binary_alts = []   # list of (level, assoc, op_node)
        atom_alts = []
        for alt in alts:
            ok = (len(alt) == 3
                  and alt[0].get('type') == 'rule'
                  and alt[0].get('value') == name
                  and 'modifier' not in alt[0]
                  and 'modifier' not in alt[1]
                  and alt[2].get('type') == 'rule'
                  and alt[2].get('value') == name
                  and 'modifier' not in alt[2])
            key = op_key(alt[1]) if ok else None
            if ok and key is not None and key in op_info:
                level, assoc = op_info[key]
                binary_alts.append((level, assoc, alt[1]))
            else:
                atom_alts.append(alt)

        if not binary_alts:
            out[name] = alts
            continue

        # Group ops by level; check assoc consistency.
        by_level = {}    # level -> list of op_node dicts
        assoc_by_level = {}
        for level, assoc, op_node in binary_alts:
            by_level.setdefault(level, []).append(op_node)
            if level in assoc_by_level and assoc_by_level[level] != assoc:
                raise SyntaxError(
                    f"rule {name!r}: operators at the same precedence "
                    f"level {level} have conflicting associativity")
            assoc_by_level[level] = assoc

        sorted_levels = sorted(by_level.keys())
        if not atom_alts:
            raise SyntaxError(
                f"rule {name!r}: precedence ladder has no atom "
                f"alternatives (an alt that isn't `{name} OP {name}`)")

        # Name plan: original rule name -> level-0 (lowest precedence).
        # Intermediate levels: _<name>_p<level>.  Atoms: _<name>_atom.
        atom_name = f"_{name}_atom"
        level_names = {}
        for i, lvl in enumerate(sorted_levels):
            level_names[lvl] = name if i == 0 else f"_{name}_p{lvl}"
        def higher_of(i):
            return level_names[sorted_levels[i + 1]] if i + 1 < len(sorted_levels) else atom_name

        for i, lvl in enumerate(sorted_levels):
            this_name = level_names[lvl]
            higher = higher_of(i)
            op_nodes = by_level[lvl]
            assoc = assoc_by_level[lvl]

            # Reuse the original op nodes verbatim so 'string' / 'rule'
            # / 'token' types are preserved in the rewritten grammar.
            op_group = {'type': 'group',
                         'alternatives': [[dict(op_node)]
                                          for op_node in op_nodes]}

            if assoc == 'left':
                # Left-recursive: LR elim will transform at build time.
                new_alts = [
                    [{'type': 'rule', 'value': this_name},
                     op_group,
                     {'type': 'rule', 'value': higher}],
                    [{'type': 'rule', 'value': higher}],
                ]
            elif assoc == 'right':
                new_alts = [
                    [{'type': 'rule', 'value': higher},
                     op_group,
                     {'type': 'rule', 'value': this_name}],
                    [{'type': 'rule', 'value': higher}],
                ]
            else:  # nonassoc
                new_alts = [
                    [{'type': 'rule', 'value': higher},
                     {'type': 'group',
                      'alternatives': [[op_group,
                                        {'type': 'rule', 'value': higher}]],
                      'modifier': '?'}],
                ]

            out[this_name] = new_alts

        out[atom_name] = atom_alts

    return out


def _apply_keywords(rules, keywords):
    """Walk the grammar AST and replace every string atom whose value
    is a declared keyword with the keyword-wrapped form.  Leaves
    modifiers (``*``, ``+`` …) intact — the wrap is inside, so the
    modifier applies to the wrapped group."""
    if not keywords:
        return rules

    def transform(elem):
        if elem.get('type') == 'string' and elem['value'] in keywords:
            wrapped = _wrap_keyword(elem['value'])
            if 'modifier' in elem:
                return {'type': 'group',
                        'alternatives': [[wrapped]],
                        'modifier': elem['modifier']}
            return wrapped
        if 'alternatives' in elem:
            elem = dict(elem)
            elem['alternatives'] = [
                [transform(e) for e in alt]
                for alt in elem['alternatives']
            ]
        return elem

    return {name: [[transform(e) for e in alt] for alt in alts]
            for name, alts in rules.items()}


# ========================== Graph Nodes ==========================

class NT:
    MATCH_CHAR  = 1   # match one specific character
    MATCH_CLASS = 2   # match one character in a CharClass
    MATCH_ANY   = 3   # match any one character
    RULE_REF    = 4
    SPLIT       = 5
    RULE_START  = 6
    RULE_END    = 7
    PRED_NOT    = 8   # !(expr) — succeed if expr does NOT match (zero-width)
    PRED_AND    = 9   # &(expr) — succeed if expr DOES match (zero-width)
    BACKREF     = 10  # match exact text previously captured by name:=(...)
    ACTION      = 11  # semantic action ({{ ... }}); node.value is Python code
    # EBNF `-` subtraction: reject if .value's sub-graph matches the span
    # from stack[-1].start_pos to the current pos exactly.  Used as the
    # second step of an anonymous stripped rule `_sub_N = A <SUB_CHECK_NOT(B)>`
    # that wraps `A - B`.
    SUB_CHECK_NOT = 12


class Node:
    _next_id = 0
    __slots__ = ('id', 'type', 'value', 'links', 'rule')

    def __init__(self, ntype, value=None, rule=None):
        self.id = Node._next_id
        Node._next_id += 1
        self.type = ntype
        self.value = value
        self.links = []
        self.rule = rule

    def __repr__(self):
        names = {1: 'CHAR', 2: 'CLASS', 3: 'ANY', 4: 'REF',
                 5: 'SPLIT', 6: 'START', 7: 'END', 8: '!', 9: '&',
                 10: 'BKREF'}
        return f"Node({self.id}, {names.get(self.type, '?')}, {self.value!r})"


# ========================== Parse Tree ==========================

class ParseNode:
    """A node in the output parse tree.

    Attributes:
        name:      rule name
        text:      the raw substring matched by this rule
        children:  sub-rule matches (list of ParseNode)
        start/end: character offsets in the input
        value:     semantic-action result (None if no action ran).
                   When a rule has a ``{{ … }}`` action, ``value`` is
                   whatever the action returned.
    """
    __slots__ = ('name', 'text', 'children', 'start', 'end', 'value')

    def __init__(self, name, text='', children=None, start=0, end=0,
                 value=None):
        self.name = name
        self.text = text
        self.children = children or []
        self.start = start
        self.end = end
        self.value = value

    def __repr__(self):
        if not self.children:
            return f"{self.name}({self.text!r})"
        kids = ', '.join(repr(c) for c in self.children)
        return f"{self.name}({kids})"

    def pretty(self, indent=0):
        prefix = '  ' * indent
        if not self.children:
            return f"{prefix}{self.name}: {self.text!r}"
        lines = [f"{prefix}{self.name}: {self.text!r}"]
        for child in self.children:
            lines.append(child.pretty(indent + 1))
        return '\n'.join(lines)


# ========================== Left-Recursion Helpers ==========================
# (same algorithms as gpda.py, copied to keep this file standalone)

def _is_lr_alt(alt, name):
    return (alt and alt[0].get('type') == 'rule'
            and alt[0].get('value') == name
            and 'modifier' not in alt[0])

def _detect_direct_lr(rules):
    lr = set()
    for name, alts in rules.items():
        for alt in alts:
            if _is_lr_alt(alt, name):
                lr.add(name)
                break
    return lr

def _transform_single_lr(alts, name):
    lr_alts, base_alts = [], []
    for alt in alts:
        if _is_lr_alt(alt, name):
            tail = alt[1:]
            if tail:
                lr_alts.append(copy.deepcopy(tail))
        else:
            base_alts.append(copy.deepcopy(alt))
    if not lr_alts or not base_alts:
        return None
    tail_name = f'_{name}_lr_tail'
    tail_ref = {'type': 'rule', 'value': tail_name, 'modifier': '*'}
    new_alts = [base + [dict(tail_ref)] for base in base_alts]
    return new_alts, tail_name, lr_alts

def _transform_lr(rules, lr_rules):
    new_rules = dict(rules)
    lr_meta = {}
    for name in lr_rules:
        r = _transform_single_lr(new_rules[name], name)
        if r:
            new_alts, tail_name, tail_alts = r
            new_rules[name] = new_alts
            new_rules[tail_name] = tail_alts
            lr_meta[name] = tail_name
    return new_rules, lr_meta

def _left_corner_graph(rules):
    graph = {n: set() for n in rules}
    for name, alts in rules.items():
        for alt in alts:
            if (alt and alt[0].get('type') == 'rule'
                    and 'modifier' not in alt[0]
                    and alt[0]['value'] in rules):
                graph[name].add(alt[0]['value'])
    return graph

def _find_sccs(graph):
    idx_ctr = [0]; stack = []; lowlink = {}; index = {}
    on_stack = set(); sccs = []
    def visit(v):
        index[v] = lowlink[v] = idx_ctr[0]; idx_ctr[0] += 1
        stack.append(v); on_stack.add(v)
        for w in graph.get(v, ()):
            if w not in index:
                visit(w); lowlink[v] = min(lowlink[v], lowlink[w])
            elif w in on_stack:
                lowlink[v] = min(lowlink[v], index[w])
        if lowlink[v] == index[v]:
            scc = []
            while True:
                w = stack.pop(); on_stack.discard(w); scc.append(w)
                if w == v: break
            sccs.append(scc)
    for v in graph:
        if v not in index: visit(v)
    return sccs

def _substitute_rule(alts, target, target_alts):
    out = []
    for alt in alts:
        if _is_lr_alt(alt, target):
            rest = alt[1:]
            for ta in target_alts:
                out.append(copy.deepcopy(ta) + copy.deepcopy(rest))
        else:
            out.append(alt)
    return out

def _eliminate_indirect_lr(rules):
    graph = _left_corner_graph(rules)
    sccs = _find_sccs(graph)
    result = dict(rules); lr_meta = {}
    rule_order = list(rules.keys())
    for scc in sccs:
        if len(scc) <= 1: continue
        scc.sort(key=lambda n: rule_order.index(n)
                 if n in rule_order else len(rule_order))
        for i, ri in enumerate(scc):
            for j in range(i):
                result[ri] = _substitute_rule(result[ri], scc[j], result[scc[j]])
            xf = _transform_single_lr(result[ri], ri)
            if xf:
                new_alts, tn, ta = xf
                result[ri] = new_alts; result[tn] = ta; lr_meta[ri] = tn
    return result, lr_meta

def _reconstruct_lr(tree, lr_meta):
    if not tree.children:
        return tree
    # Recurse into children.  Track whether any child actually changed —
    # if nothing did and we're not an LR rule, we can return `tree`
    # unchanged and skip allocating a copy.  For grammars where LR rules
    # are a small fraction of the tree (e.g. arith: LR only at `expr`,
    # but the tree is mostly nested `term`/digit subtrees), this turns
    # an O(N) copy pass into a no-op over the non-LR subtrees.
    any_changed = False
    children = []
    for c in tree.children:
        nc = _reconstruct_lr(c, lr_meta)
        if nc is not c:
            any_changed = True
        children.append(nc)
    if tree.name not in lr_meta:
        if not any_changed:
            return tree
        return ParseNode(tree.name, tree.text, children, tree.start, tree.end)
    tail_name = lr_meta[tree.name]
    base, tails = [], []
    for c in children:
        (tails if c.name == tail_name else base).append(c)
    if not tails:
        return ParseNode(tree.name, tree.text, base, tree.start, tree.end)
    # Compute text for reconstructed intermediate nodes as a slice of
    # the outer tree's text — intermediate nodes wouldn't otherwise have
    # populated text (no access to raw input here).  Offsets are
    # absolute, so subtract the outer tree's start.
    outer_text = tree.text
    outer_start = tree.start
    def text_slice(s, e):
        return outer_text[s - outer_start : e - outer_start]
    cur_start = base[0].start if base else tree.start
    cur_end = base[-1].end if base else tree.start
    current = ParseNode(tree.name, text_slice(cur_start, cur_end),
                        list(base), cur_start, cur_end)
    for tail in tails:
        current = ParseNode(tree.name,
                            text_slice(current.start, tail.end),
                            [current] + list(tail.children),
                            current.start, tail.end)
    current.text = tree.text
    return current


# ========================== Auto-Whitespace Insertion ==========================

def _insert_ws(rules, skip_rules=frozenset(), atomic_rules=frozenset(),
               stripped_out=None, rule_modes=None, skip_modes=None):
    """Auto-insert a skip-rule reference between consecutive elements in
    non-atomic rules.

    Skip rules (rules named `_` or marked `@skip`) are partitioned by
    their mode tag (via `@mode <name>`, default mode name = 'default').
    For each mode present, a synthetic `_skip_<mode>` rule is built as
    ``(skip1 | skip2 | ...)*``.  At insertion time, each rule's own
    mode selects which synthetic skip rule is inserted inside it.

    Backward compat (no @mode tags anywhere):
        All skip rules land in the 'default' mode, which is exactly how
        the pre-@mode version worked.  If the only skip rule is `_` and
        no user `@skip` rules exist, insert `_` directly (no synthesis).

    Atomic rules (no insertion inside them):
        * any skip rule (to avoid auto-ws inside the ws definition itself)
        * ALL-CAPS rule names
        * rules marked `@atomic`
        * rules whose mode has no matching skip rules
    """
    rule_modes = rule_modes or {}
    skip_modes = skip_modes or {}

    # Partition skip rules by mode.
    by_mode = {}
    for name in skip_rules:
        m = skip_modes.get(name, 'default')
        by_mode.setdefault(m, []).append(name)
    # The traditional `_` rule goes into the default mode.
    has_underscore = '_' in rules
    if has_underscore:
        default_list = by_mode.setdefault('default', [])
        if '_' not in default_list:
            default_list.insert(0, '_')
    if not by_mode:
        return rules

    rules = dict(rules)
    insert_name_by_mode = {}     # mode -> rule name to insert
    for mode, mode_skip_rules in by_mode.items():
        # Backward-compat shortcut: if the default mode's only skip
        # rule is `_` (no user @skip tagged to default), insert `_`
        # directly.  Requires `_` to already be zero-or-more.
        if (mode == 'default' and mode_skip_rules == ['_']
                and not any(skip_modes.get(r, 'default') == 'default'
                            for r in skip_rules)):
            insert_name_by_mode[mode] = '_'
            continue
        synth_name = f'_skip_{mode}' if mode != 'default' else '_skip'
        while synth_name in rules:
            synth_name += '_'
        rules[synth_name] = [[{
            'type': 'group',
            'alternatives': [[{'type': 'rule', 'value': r}]
                             for r in mode_skip_rules],
            'modifier': '*',
        }]]
        insert_name_by_mode[mode] = synth_name
        if stripped_out is not None:
            stripped_out.add(synth_name)

    # Report all skip rule names (user-declared + `_`) as stripped.
    if stripped_out is not None:
        for lst in by_mode.values():
            stripped_out.update(lst)

    # Names we treat as "already-ws" for suppression adjacent to explicit
    # references (all skip rules across all modes, plus the synth names).
    skip_set = set(insert_name_by_mode.values())
    for lst in by_mode.values():
        skip_set.update(lst)

    def is_ws(elem):
        return elem.get('type') == 'rule' and elem.get('value') in skip_set

    def insert_in_seq(elements, atomic, insert_name):
        if atomic or len(elements) <= 1:
            return [process(e, atomic, insert_name) for e in elements]
        out = []
        for i, e in enumerate(elements):
            if i > 0 and not is_ws(elements[i - 1]) and not is_ws(e):
                out.append({'type': 'rule', 'value': insert_name})
            out.append(process(e, atomic, insert_name))
        return out

    def process(elem, atomic, insert_name):
        if 'alternatives' in elem:
            # An element can opt out of auto-ws for its inner sequences
            # by setting atomic=True (e.g. the @keyword-wrap group).
            inner_atomic = atomic or elem.get('atomic', False)
            elem = dict(elem)
            elem['alternatives'] = [
                insert_in_seq(alt, inner_atomic, insert_name)
                for alt in elem['alternatives']
            ]
        return elem

    new_rules = {}
    for name, alts in rules.items():
        mode = rule_modes.get(name, 'default')
        insert_name = insert_name_by_mode.get(mode)
        atomic = ((name in skip_set)
                  or (name.isupper() and bool(name))
                  or (name in atomic_rules)
                  or (insert_name is None))
        if insert_name is None:
            insert_name = '_skip'  # placeholder; unused because atomic=True
        new_rules[name] = [insert_in_seq(alt, atomic, insert_name)
                           for alt in alts]
    return new_rules


# ========================== Regularity Analysis & DFA Compilation ==========

def _walk_rule(start):
    """Return all graph nodes reachable from ``start`` via .links,
    staying within this rule (RULE_REF's .links go to continuation
    nodes in the same rule, so we naturally don't cross boundaries)."""
    seen = set()
    stack = [start]
    while stack:
        n = stack.pop()
        if n in seen:
            continue
        seen.add(n)
        stack.extend(n.links)
    return seen


def _analyze_regularity(rules, capture_names):
    """Identify rule names whose subgraph is DFA-compilable.

    A rule is regular iff:
        - Its subgraph uses only MATCH_CHAR, MATCH_CLASS, MATCH_ANY,
          SPLIT, RULE_START, RULE_END, and RULE_REF.  Predicates,
          backrefs, and capture-inline rules are disqualifying.
        - Every RULE_REF target is also regular.
        - The rule-ref subgraph among regular rules is acyclic
          (regular languages can't express recursive structure — any
          cycle disqualifies all members).
    """
    rule_nodes = {}
    rule_refs = {}
    for name, (start, _end) in rules.items():
        nodes = _walk_rule(start)
        rule_nodes[name] = nodes
        rule_refs[name] = {n.value for n in nodes if n.type == NT.RULE_REF}

    def has_bad(nodes):
        for n in nodes:
            if n.type in (NT.PRED_AND, NT.PRED_NOT, NT.BACKREF):
                return True
        return False

    candidates = {
        name for name, nodes in rule_nodes.items()
        if not has_bad(nodes) and name not in capture_names
    }

    def reaches_self(start, current):
        visited = set()
        stack = [r for r in rule_refs.get(start, ()) if r in current]
        while stack:
            n = stack.pop()
            if n == start:
                return True
            if n in visited:
                continue
            visited.add(n)
            stack.extend(r for r in rule_refs.get(n, ()) if r in current)
        return False

    # Iterate: each round drops rules that (a) ref a non-candidate or
    # (b) participate in a rule-ref cycle among remaining candidates.
    # Removing cyclic rules can expose further rules that now ref a
    # removed rule, so we loop until a fixed point.
    while True:
        changed = False
        for name in list(candidates):
            if not rule_refs[name] <= candidates:
                candidates.discard(name); changed = True
        cyclic = {n for n in candidates if reaches_self(n, candidates)}
        if cyclic:
            candidates -= cyclic
            changed = True
        if not changed:
            break

    return candidates


class Dfa:
    """A DFA compiled from a regular rule.  Transition tables index by
    byte value; unsupported bytes (>= 128 in the current compiler) map
    to state -1 meaning "no transition".

    Attributes:
        num_states: total number of DFA states (state 0 is the start).
        trans: list of lists, `trans[s][byte]` = next state or -1.
        accept: list of bools, `accept[s]` = True iff state s
                represents a completed match of the rule.
        longest: True if only the longest match position should be
                emitted at parse time (the rule was declared
                ``@longest``).  Default False — emit every accept
                position, matching GPDA's default all-paths semantics.
    """
    __slots__ = ('num_states', 'trans', 'accept', 'rule_name', 'longest')

    def __init__(self, num_states, rule_name=None, longest=False):
        self.num_states = num_states
        self.trans = [[-1] * 256 for _ in range(num_states)]
        self.accept = [False] * num_states
        self.rule_name = rule_name
        self.longest = longest

    def match_all(self, text, start=0):
        """Run the DFA over ``text`` beginning at ``start``.

        If ``longest`` is False (default), returns all positions where
        the DFA reached an accept state.  If True, returns a single-
        element list containing the longest such position (or an empty
        list if no match)."""
        out = []
        s = 0
        if self.accept[s]:
            out.append(start)
        i = start
        while i < len(text):
            c = ord(text[i])
            if c >= 256:
                break
            nxt = self.trans[s][c]
            if nxt < 0:
                break
            s = nxt
            i += 1
            if self.accept[s]:
                out.append(i)
        if self.longest:
            return [out[-1]] if out else []
        return out


def _compile_dfa(rule_name, rules, regular_rules, longest=False):
    """Convert one regular rule's NFA subgraph into a DFA via powerset
    construction.  Inlines references to other regular rules (safe
    because the ref-subgraph among regular rules is acyclic)."""
    start_node, top_end = rules[rule_name]

    # A state is a frozenset of (node_id, call_stack) pairs.  call_stack
    # is a tuple of RULE_REF nodes we've entered (but not yet returned
    # from), so we know where to resume after the inlined rule's
    # RULE_END.  Nodes are hashable by identity; we store the node
    # object itself and use id()-based hashing via the default
    # Python __hash__.
    def ec(initial):
        out = set(initial)
        stack = list(initial)
        while stack:
            node, call = stack.pop()
            t = node.type
            if t == NT.SPLIT or t == NT.RULE_START:
                for link in node.links:
                    p = (link, call)
                    if p not in out:
                        out.add(p); stack.append(p)
            elif t == NT.RULE_REF:
                if node.value in regular_rules:
                    ref_start, _ = rules[node.value]
                    p = (ref_start, call + (node,))
                    if p not in out:
                        out.add(p); stack.append(p)
                # else: kept in the state; a non-regular rule ref in
                # an epsilon closure makes this state non-advanceable
                # for DFA purposes — we'll detect it below.
            elif t == NT.RULE_END:
                if call:
                    caller = call[-1]
                    new_call = call[:-1]
                    for link in caller.links:
                        p = (link, new_call)
                        if p not in out:
                            out.add(p); stack.append(p)
                # Empty call_stack: top-level rule exit.  Keep it in
                # the state (used for the accept check).
            # MATCH_* nodes: stop epsilon walking here.
        return frozenset(out)

    def is_accept(state):
        return any(n is top_end and c == () for n, c in state)

    start_state = ec({(start_node, ())})
    states = {start_state: 0}
    dfa = Dfa(num_states=1, rule_name=rule_name, longest=longest)
    dfa.accept[0] = is_accept(start_state)

    worklist = [start_state]
    while worklist:
        state = worklist.pop()
        sid = states[state]

        consumers = [(n, c) for n, c in state
                     if n.type in (NT.MATCH_CHAR, NT.MATCH_CLASS, NT.MATCH_ANY)]

        # Collect next-state for each byte 0..255.  Adjacent bytes that
        # map to the same set of next nodes collapse automatically when
        # their ec() results hash equal.
        for ch in range(256):
            ch_str = chr(ch)
            next_nodes = set()
            for n, c in consumers:
                if n.type == NT.MATCH_CHAR and n.value == ch_str:
                    for link in n.links:
                        next_nodes.add((link, c))
                elif n.type == NT.MATCH_CLASS and n.value.matches(ch_str):
                    for link in n.links:
                        next_nodes.add((link, c))
                elif n.type == NT.MATCH_ANY:
                    for link in n.links:
                        next_nodes.add((link, c))
            if not next_nodes:
                continue
            next_state = ec(next_nodes)
            if next_state not in states:
                states[next_state] = len(states)
                dfa.num_states += 1
                dfa.trans.append([-1] * 256)
                dfa.accept.append(is_accept(next_state))
                worklist.append(next_state)
            dfa.trans[sid][ch] = states[next_state]

    return dfa


def _compile_all_dfas(rules, regular_rules, longest_rules=frozenset()):
    """Compile a Dfa for each regular rule.  Returns dict name -> Dfa.
    Rules in ``longest_rules`` are flagged as longest-match on their
    emitted Dfa."""
    return {name: _compile_dfa(name, rules, regular_rules,
                                longest=(name in longest_rules))
            for name in regular_rules}


# ========================== Graph Builder ==========================

class GraphBuilder:
    """Converts a unified grammar dict into a character-level graph."""

    def __init__(self):
        self.rules = {}
        self.start_rule = None
        self.lr_meta = {}
        self.regular_rules = set()
        self.dfas = {}
        self.stripped_names = set()
        self._sub_counter = 0  # names synthesized for `A - B` anon rules

    def build(self, grammar):
        rules_dict = grammar['rules']
        self.start_rule = grammar['start']
        skip_rules = grammar.get('skip_rules', set())
        atomic_rules = grammar.get('atomic_rules', set())
        keywords = grammar.get('keywords', set())
        rule_modes = grammar.get('rule_modes', {})
        skip_modes = grammar.get('skip_modes', {})

        # Apply precedence declarations first: rewrites ambiguous
        # `R = R op1 R | R op2 R | atom_alts` rules into a precedence
        # ladder (`R = R_0`, `R_0 = R_0 op1_list R_1 | R_1`, ...,
        # `R_N = atom_alts`) so downstream passes see an unambiguous
        # grammar that dedup can collapse cursors on.
        precedence = grammar.get('precedence', [])
        if precedence:
            rules_dict = _apply_precedence(rules_dict, precedence)

        # Auto-insert a skip-rule reference between consecutive elements
        # in non-atomic rules.  Do this before LR transform so the
        # transform operates on the augmented grammar.  _insert_ws also
        # reports which rule names should be "stripped" (their matches
        # invisible in the parse tree output).
        self.stripped_names = set()
        rules_dict = _insert_ws(rules_dict, skip_rules=skip_rules,
                                 atomic_rules=atomic_rules,
                                 stripped_out=self.stripped_names,
                                 rule_modes=rule_modes,
                                 skip_modes=skip_modes)

        # Apply @keyword keyword wrapping AFTER auto-ws insertion so the
        # wrapped group `('class' !wordchar)` doesn't itself receive
        # an inserted `_` between its atoms.
        rules_dict = _apply_keywords(rules_dict, keywords)

        rules_dict, lr1 = _eliminate_indirect_lr(rules_dict)
        lr_rules = _detect_direct_lr(rules_dict)
        if lr_rules:
            rules_dict, lr2 = _transform_lr(rules_dict, lr_rules)
        else:
            lr2 = {}
        self.lr_meta = {**lr1, **lr2}

        # Scan each rule for capture names (for backref resolution)
        self._rule_caps = {}
        self.capture_names = set()
        for name, alts in rules_dict.items():
            caps = set()
            for alt in alts:
                self._collect_captures(alt, caps)
            self._rule_caps[name] = caps
            self.capture_names.update(caps)

        for name in rules_dict:
            self.rules[name] = (Node(NT.RULE_START, rule=name),
                                Node(NT.RULE_END, rule=name))
        for name, alts in rules_dict.items():
            self._cur_caps = self._rule_caps.get(name, set())
            start, end = self.rules[name]
            for alt in alts:
                cs, ce = self._build_seq(alt)
                start.links.append(cs)
                ce.links.append(end)

        # Identify which rules are DFA-compilable and build their DFAs.
        # DFAs aren't yet wired into the Python parser runtime; they're
        # data the C++ emitter picks up for the fast-path there.  Keep
        # the Python parse behavior unchanged.
        self.regular_rules = _analyze_regularity(self.rules,
                                                  self.capture_names)
        longest_rules = grammar.get('longest_rules', set())
        # @longest on a non-regular rule is an error — we can't express
        # longest-only semantics without a DFA in the current runtime.
        bad = longest_rules - self.regular_rules
        if bad:
            raise SyntaxError(
                f"@longest is only valid on DFA-compilable (regular) "
                f"rules; offending rule(s): {sorted(bad)}")
        self.dfas = _compile_all_dfas(self.rules, self.regular_rules,
                                       longest_rules=longest_rules)

        return (self.rules, self.start_rule, self.lr_meta,
                self.capture_names, self.dfas, self.stripped_names)

    @staticmethod
    def _collect_captures(elements, result):
        """Recursively find capture names in a list of elements."""
        for elem in elements:
            if elem.get('type') == 'capture':
                result.add(elem['name'])
            for alt in elem.get('alternatives', []):
                GraphBuilder._collect_captures(alt, result)

    def _build_seq(self, elements):
        if not elements:
            n = Node(NT.SPLIT); return n, n
        first_s, first_e = self._build_elem(elements[0])
        prev_e = first_e
        for elem in elements[1:]:
            s, e = self._build_elem(elem)
            prev_e.links.append(s)
            prev_e = e
        return first_s, prev_e

    def _build_elem(self, elem):
        atom_s, atom_e = self._build_atom(elem)
        mod = elem.get('modifier')
        if not mod:
            return atom_s, atom_e
        if mod == '*':  return self._star(atom_s, atom_e, True)
        if mod == '*?': return self._star(atom_s, atom_e, False)
        if mod == '+':  return self._plus(atom_s, atom_e, True)
        if mod == '+?': return self._plus(atom_s, atom_e, False)
        if mod == '?':  return self._opt(atom_s, atom_e, True)
        if mod == '??': return self._opt(atom_s, atom_e, False)
        if isinstance(mod, dict):
            return self._repeat(atom_s, atom_e, mod, elem)
        return atom_s, atom_e

    def _build_atom(self, elem):
        t = elem['type']
        if t == 'string':
            chars = elem['value']
            if not chars:
                n = Node(NT.SPLIT); return n, n
            nodes = [Node(NT.MATCH_CHAR, c) for c in chars]
            for i in range(len(nodes) - 1):
                nodes[i].links.append(nodes[i + 1])
            return nodes[0], nodes[-1]
        if t == 'charclass':
            n = Node(NT.MATCH_CLASS, elem['charclass'])
            return n, n
        if t == 'any':
            n = Node(NT.MATCH_ANY)
            return n, n
        if t == 'rule':
            name = elem['value']
            if name in getattr(self, '_cur_caps', ()):
                n = Node(NT.BACKREF, name)
                return n, n
            n = Node(NT.RULE_REF, name)
            return n, n
        if t == 'group':
            return self._build_alts(elem['alternatives'])
        if t == 'capture':
            # Named capture → inline rule
            name = elem['name']
            start = Node(NT.RULE_START, rule=name)
            end = Node(NT.RULE_END, rule=name)
            inner_s, inner_e = self._build_alts(elem['alternatives'])
            start.links.append(inner_s)
            inner_e.links.append(end)
            self.rules[name] = (start, end)
            ref = Node(NT.RULE_REF, name)
            return ref, ref
        if t in ('and', 'not'):
            # & / ! predicate → sub-graph checked without consuming input
            pred_start = Node(NT.RULE_START, rule='_pred')
            pred_end = Node(NT.RULE_END, rule='_pred')
            inner_s, inner_e = self._build_alts(elem['alternatives'])
            pred_start.links.append(inner_s)
            inner_e.links.append(pred_end)
            ptype = NT.PRED_AND if t == 'and' else NT.PRED_NOT
            pred = Node(ptype, value=pred_start)
            return pred, pred
        if t == 'action':
            n = Node(NT.ACTION, elem['code'])
            return n, n
        if t == 'subtract':
            # EBNF `A - B`: match A, reject if B also matches the same span.
            # Implementation: wrap in an anonymous stripped rule
            #   _sub_N = A  <SUB_CHECK_NOT(B)>
            # so the rule's start_pos on the cursor stack is A's start.
            # SUB_CHECK_NOT runs B as a bounded sub-parse from start_pos
            # to the current pos; if B reaches completion exactly at the
            # current pos, the cursor fails.
            anon = f'_sub_{self._sub_counter}'
            self._sub_counter += 1
            rs = Node(NT.RULE_START, rule=anon)
            re_ = Node(NT.RULE_END, rule=anon)
            self.rules[anon] = (rs, re_)
            self.stripped_names.add(anon)

            a_s, a_e = self._build_elem(elem['left'])

            # B's sub-graph is wrapped in its own RULE_START/RULE_END so
            # _evaluate_predicate_bounded can walk it independently.
            b_pred_s = Node(NT.RULE_START, rule='_sub_pred')
            b_pred_e = Node(NT.RULE_END, rule='_sub_pred')
            b_inner_s, b_inner_e = self._build_elem(elem['right'])
            b_pred_s.links.append(b_inner_s)
            b_inner_e.links.append(b_pred_e)

            check = Node(NT.SUB_CHECK_NOT, value=b_pred_s)
            rs.links.append(a_s)
            a_e.links.append(check)
            check.links.append(re_)

            ref = Node(NT.RULE_REF, anon)
            return ref, ref
        raise ValueError(f"Unknown element type: {t}")

    def _build_alts(self, alternatives):
        if len(alternatives) == 1:
            return self._build_seq(alternatives[0])
        split = Node(NT.SPLIT)
        join = Node(NT.SPLIT)
        for alt in alternatives:
            s, e = self._build_seq(alt)
            split.links.append(s)
            e.links.append(join)
        return split, join

    def _star(self, s, e, greedy):
        entry = Node(NT.SPLIT); exit_ = Node(NT.SPLIT)
        entry.links = [s, exit_] if greedy else [exit_, s]
        e.links.append(entry); return entry, exit_

    def _plus(self, s, e, greedy):
        loop = Node(NT.SPLIT); exit_ = Node(NT.SPLIT)
        e.links.append(loop)
        loop.links = [s, exit_] if greedy else [exit_, s]
        return s, exit_

    def _opt(self, s, e, greedy):
        entry = Node(NT.SPLIT); exit_ = Node(NT.SPLIT)
        entry.links = [s, exit_] if greedy else [exit_, s]
        e.links.append(exit_); return entry, exit_

    def _repeat(self, s, e, mod, elem):
        lo = mod.get('min', 0); hi = mod.get('max', lo)
        greedy = mod.get('greedy', True)
        parts = []
        for _ in range(lo):
            cs, ce = self._build_atom(elem); parts.append((cs, ce))
        for i in range(len(parts) - 1):
            parts[i][1].links.append(parts[i + 1][0])
        # Unbounded upper ({n,}): after lo mandatory copies, attach a
        # Kleene-star loop for 0-or-more additional copies.
        if hi == -1:
            cs, ce = self._build_atom(elem)
            star_s, star_e = self._star(cs, ce, greedy)
            if parts:
                parts[-1][1].links.append(star_s)
                return parts[0][0], star_e
            return star_s, star_e
        opt_parts = []
        for _ in range(hi - lo):
            cs, ce = self._build_atom(elem)
            os, oe = self._opt(cs, ce, greedy); opt_parts.append((os, oe))
        for i in range(len(opt_parts) - 1):
            opt_parts[i][1].links.append(opt_parts[i + 1][0])
        exit_ = Node(NT.SPLIT)
        if opt_parts:
            opt_parts[-1][1].links.append(exit_)
            if parts:
                parts[-1][1].links.append(opt_parts[0][0])
                return parts[0][0], exit_
            return opt_parts[0][0], exit_
        if parts:
            parts[-1][1].links.append(exit_)
            return parts[0][0], exit_
        return exit_, exit_


# ========================== Scannerless Parser ==========================

class ScannerlessParser:
    """Graph-walking parser that operates on raw characters.

    Cursor tuple: ``(node, stack, children, captures)``

    ``captures`` is a dict mapping capture names to their matched text.
    It flows through the cursor so that backreference nodes (BACKREF)
    can match against previously captured text.
    """

    MAX_DEPTH = 200

    def __init__(self, rules, start_rule, lr_meta=None, capture_names=None,
                 dfas=None, stripped_names=None):
        self.rules = rules
        self.start_rule = start_rule
        self.lr_meta = lr_meta or {}
        self.capture_names = capture_names or set()
        # DFAs for regular rules.  Currently the Python parse loop doesn't
        # use them (the speedup lives in C++); we retain them here so
        # callers like the C++ emitter can read them off the parser.
        self.dfas = dfas or {}
        # Rule names whose matches are invisible in the parse tree output
        # (auto-ws `_`, user-declared @skip rules, synthesised _skip).
        self.stripped_names = stripped_names or set()

    def parse(self, text):
        self._text = text
        start_node = self.rules[self.start_rule][0]
        n = len(text)

        # Ordered-choice disambiguation is implicit: _expand traverses
        # links in order (link[0] first), and _dedup keeps the first
        # cursor to reach any given state.  So the cursor representing
        # the earliest-listed alternative naturally wins.
        cursors = [(start_node, (), [], {})]
        # Deferred cursors placed at future positions by multi-char backrefs
        deferred = {}  # target_pos → [cursor, ...]
        furthest = 0

        for pos in range(n):
            if pos in deferred:
                cursors = self._dedup(list(cursors) + deferred.pop(pos))

            if not cursors:
                continue

            furthest = pos
            ch = text[pos]
            expanded = self._expand_all(cursors, pos)

            next_cursors = []
            for term_node, stack, children, caps in expanded:
                if term_node.type == NT.BACKREF:
                    ref = caps.get(term_node.value, '')
                    end = pos + len(ref)
                    if end <= n and text[pos:end] == ref:
                        for link in term_node.links:
                            deferred.setdefault(end, []).append(
                                (link, stack, children, caps))
                elif self._char_matches(term_node, ch):
                    for link in term_node.links:
                        next_cursors.append(
                            (link, stack, children, caps))

            cursors = self._dedup(next_cursors)

        # Include deferred cursors arriving at the end
        if n in deferred:
            cursors = self._dedup(list(cursors) + deferred.pop(n))

        completions = self._find_completions(cursors, n)
        if not completions:
            if furthest < n:
                ch = text[furthest]
                line = text[:furthest].count('\n') + 1
                col = furthest - text[:furthest].rfind('\n')
                raise SyntaxError(
                    f"Unexpected {ch!r} at line {line}, col {col} "
                    f"(pos {furthest})")
            raise SyntaxError(
                f"Unexpected end of input at position {n}")

        tree = completions[0]
        if self.lr_meta:
            tree = _reconstruct_lr(tree, self.lr_meta)
        return tree

    # ---- expansion ---------------------------------------------------

    def _expand_all(self, cursors, pos):
        results = []
        for node, stack, children, caps in cursors:
            self._expand(node, stack, children, pos, caps,
                         results, set())
        return results

    def _expand(self, node, stack, children, pos, caps,
                results, visited):
        if len(stack) > self.MAX_DEPTH:
            return
        stack_key = tuple((r, rl, sp) for r, rl, sp, *_ in stack)
        cap_key = tuple(sorted(caps.items())) if caps else ()
        key = (node.id, stack_key, cap_key)
        if key in visited:
            return
        visited.add(key)

        nt = node.type
        if nt == NT.SPLIT or nt == NT.RULE_START:
            for link in node.links:
                self._expand(link, stack, children, pos, caps,
                             results, visited)
        elif nt == NT.RULE_REF:
            name = node.value
            if name not in self.rules:
                raise ValueError(f"Unknown rule: {name!r}")
            rs = self.rules[name][0]
            # Entering a rule starts a fresh capture scope so the callee's
            # captures don't collide with the caller's.  The caller's caps
            # are saved on the stack and restored at RULE_END.
            new_stack = stack + ((name, tuple(node.links), pos, children,
                                  caps),)
            self._expand(rs, new_stack, [], pos, {},
                         results, visited)
        elif nt == NT.RULE_END:
            if stack:
                (rname, ret_links, start_pos, parent_ch,
                 parent_caps) = stack[-1]
                new_stack = stack[:-1]
                action_result = caps.get('__result__', None)
                # If this rule has no explicit action but has exactly
                # one child with a value (typical of a capture rule
                # wrapping another rule), inherit that value.  Mirrors
                # bison's default `$$ = $1` behaviour.
                effective_value = action_result
                if effective_value is None and len(children) == 1 \
                        and children[0].value is not None:
                    effective_value = children[0].value
                if rname in self.stripped_names:
                    new_ch = parent_ch
                else:
                    rnode = ParseNode(rname, self._text[start_pos:pos],
                                     list(children), start_pos, pos,
                                     value=effective_value)
                    new_ch = parent_ch + [rnode]
                new_caps = parent_caps
                if rname in self.capture_names:
                    cap_val = (effective_value if effective_value is not None
                               else self._text[start_pos:pos])
                    new_caps = {**parent_caps, rname: cap_val}
                for link in ret_links:
                    self._expand(link, new_stack, new_ch, pos,
                                 new_caps, results, visited)
        elif nt in (NT.PRED_NOT, NT.PRED_AND):
            matches = self._evaluate_predicate(node.value, pos, caps)
            passes = (nt == NT.PRED_AND and matches) or \
                     (nt == NT.PRED_NOT and not matches)
            if passes:
                for link in node.links:
                    self._expand(link, stack, children, pos, caps,
                                 results, visited)
        elif nt == NT.BACKREF:
            ref = caps.get(node.value)
            if ref is not None:
                if len(ref) == 0:
                    for link in node.links:
                        self._expand(link, stack, children, pos, caps,
                                     results, visited)
                else:
                    results.append((node, stack, children, caps))
        elif nt == NT.ACTION:
            start_pos = stack[-1][2] if stack else 0
            scope = {k: v for k, v in caps.items() if not k.startswith('__')}
            scope['_text']  = self._text[start_pos:pos]
            scope['_start'] = start_pos
            scope['_end']   = pos
            try:
                result = eval(node.value, _ACTION_GLOBALS, scope)
            except Exception as e:
                raise RuntimeError(
                    f"semantic action error: {e!r} in action "
                    f"{node.value!r}") from e
            new_caps = {**caps, '__result__': result}
            for link in node.links:
                self._expand(link, stack, children, pos, new_caps,
                             results, visited)
        elif nt == NT.SUB_CHECK_NOT:
            # EBNF A - B: stack[-1].start_pos is A's start, pos is A's end.
            # Reject this cursor if B matches the span [start_pos, pos).
            if not stack:
                return
            start_pos = stack[-1][2]
            if self._evaluate_predicate_bounded(node.value, start_pos, pos):
                return
            for link in node.links:
                self._expand(link, stack, children, pos, caps,
                             results, visited)
        elif nt in (NT.MATCH_CHAR, NT.MATCH_CLASS, NT.MATCH_ANY):
            results.append((node, stack, children, caps))

    # ---- predicate evaluation ----------------------------------------

    def _evaluate_predicate(self, pred_start, pos, caps):
        self._pred_depth = getattr(self, '_pred_depth', 0) + 1
        if self._pred_depth > 50:
            self._pred_depth -= 1
            return False
        try:
            cursors = [(pred_start, (), [], caps)]
            # Deferred cursors scheduled at future positions by multi-
            # char backrefs.  Same pattern as parse()'s main loop.
            deferred = {}
            if self._find_completions(cursors, pos):
                return True
            for i in range(pos, len(self._text)):
                if i in deferred:
                    cursors = self._dedup(list(cursors) + deferred.pop(i))
                    # A freshly woken deferred cursor may reach RULE_END
                    # of the predicate zero-width (the whole predicate
                    # body was consumed by the backref).  Check here —
                    # the usual find_completions call below is at i+1.
                    if cursors and self._find_completions(cursors, i):
                        return True
                if not cursors:
                    if not deferred:
                        return False
                    continue
                expanded = self._expand_all(cursors, i)
                next_c = []
                ch = self._text[i]
                for tnode, stk, chld, cp in expanded:
                    if tnode.type == NT.BACKREF:
                        ref = cp.get(tnode.value, '')
                        end = i + len(ref)
                        if end <= len(self._text) \
                                and self._text[i:end] == ref:
                            for link in tnode.links:
                                deferred.setdefault(end, []).append(
                                    (link, stk, chld, cp))
                    elif self._char_matches(tnode, ch):
                        for link in tnode.links:
                            next_c.append((link, stk, chld, cp))
                if not next_c and not deferred:
                    return False
                cursors = self._dedup(next_c)
                if self._find_completions(cursors, i + 1):
                    return True
            if len(self._text) in deferred:
                cursors = self._dedup(list(cursors)
                                       + deferred.pop(len(self._text)))
            return bool(self._find_completions(cursors, len(self._text)))
        finally:
            self._pred_depth -= 1

    def _evaluate_predicate_bounded(self, pred_start, start_pos, end_pos):
        """Return True iff the sub-graph at *pred_start* matches the
        exact span [start_pos, end_pos).  Used to implement EBNF `A - B`:
        rejects A's match when B also matches the same span."""
        self._pred_depth = getattr(self, '_pred_depth', 0) + 1
        if self._pred_depth > 50:
            self._pred_depth -= 1
            return False
        try:
            cursors = [(pred_start, (), [], {})]
            if start_pos == end_pos:
                return bool(self._find_completions(cursors, start_pos))
            # Deferred cursors for multi-char backref advancement (EEBNF
            # `A - B` where B contains a backref).
            deferred = {}
            for i in range(start_pos, end_pos):
                if i in deferred:
                    cursors = self._dedup(list(cursors) + deferred.pop(i))
                if not cursors:
                    if not deferred:
                        return False
                    continue
                # For the bounded predicate we only care about completion
                # at exactly end_pos; no mid-iteration completion check.
                expanded = self._expand_all(cursors, i)
                next_c = []
                ch = self._text[i]
                for tnode, stk, chld, cp in expanded:
                    if tnode.type == NT.BACKREF:
                        ref = cp.get(tnode.value, '')
                        end = i + len(ref)
                        if end <= end_pos \
                                and self._text[i:end] == ref:
                            for link in tnode.links:
                                deferred.setdefault(end, []).append(
                                    (link, stk, chld, cp))
                    elif self._char_matches(tnode, ch):
                        for link in tnode.links:
                            next_c.append((link, stk, chld, cp))
                if not next_c and not deferred:
                    return False
                cursors = self._dedup(next_c)
            if end_pos in deferred:
                cursors = self._dedup(list(cursors) + deferred.pop(end_pos))
            return bool(self._find_completions(cursors, end_pos))
        finally:
            self._pred_depth -= 1

    # ---- completion --------------------------------------------------

    def _find_completions(self, cursors, pos):
        results = []
        for node, stack, children, caps in cursors:
            self._find_completion(node, stack, children, pos, caps,
                                  results, set())
        return results

    def _find_completion(self, node, stack, children, pos, caps,
                         results, visited):
        stack_key = tuple((r, rl, sp) for r, rl, sp, *_ in stack)
        cap_key = tuple(sorted(caps.items())) if caps else ()
        key = (node.id, stack_key, cap_key)
        if key in visited:
            return
        visited.add(key)

        nt = node.type
        if nt == NT.SPLIT or nt == NT.RULE_START:
            for link in node.links:
                self._find_completion(link, stack, children, pos,
                                      caps, results, visited)
        elif nt == NT.RULE_REF:
            name = node.value
            if name in self.rules:
                rs = self.rules[name][0]
                ns = stack + ((name, tuple(node.links), pos, children,
                               caps),)
                self._find_completion(rs, ns, [], pos, {},
                                      results, visited)
        elif nt in (NT.PRED_NOT, NT.PRED_AND):
            matches = self._evaluate_predicate(node.value, pos, caps)
            passes = (nt == NT.PRED_AND and matches) or \
                     (nt == NT.PRED_NOT and not matches)
            if passes:
                for link in node.links:
                    self._find_completion(link, stack, children, pos,
                                          caps, results, visited)
        elif nt == NT.BACKREF:
            ref = caps.get(node.value, '')
            if len(ref) == 0:
                for link in node.links:
                    self._find_completion(link, stack, children, pos, caps,
                                          results, visited)
        elif nt == NT.ACTION:
            start_pos = stack[-1][2] if stack else 0
            scope = {k: v for k, v in caps.items() if not k.startswith('__')}
            scope['_text']  = self._text[start_pos:pos]
            scope['_start'] = start_pos
            scope['_end']   = pos
            try:
                result = eval(node.value, _ACTION_GLOBALS, scope)
            except Exception as e:
                raise RuntimeError(
                    f"semantic action error: {e!r} in action "
                    f"{node.value!r}") from e
            new_caps = {**caps, '__result__': result}
            for link in node.links:
                self._find_completion(link, stack, children, pos, new_caps,
                                      results, visited)
        elif nt == NT.SUB_CHECK_NOT:
            if not stack:
                return
            start_pos = stack[-1][2]
            if self._evaluate_predicate_bounded(node.value, start_pos, pos):
                return
            for link in node.links:
                self._find_completion(link, stack, children, pos, caps,
                                      results, visited)
        elif nt == NT.RULE_END:
            if stack:
                (rname, ret_links, start_pos, parent_ch,
                 parent_caps) = stack[-1]
                new_stack = stack[:-1]
                action_result = caps.get('__result__', None)
                effective_value = action_result
                if effective_value is None and len(children) == 1 \
                        and children[0].value is not None:
                    effective_value = children[0].value
                if rname in self.stripped_names:
                    new_ch = parent_ch
                else:
                    rnode = ParseNode(rname, self._text[start_pos:pos],
                                     list(children), start_pos, pos,
                                     value=effective_value)
                    new_ch = parent_ch + [rnode]
                new_caps = parent_caps
                if rname in self.capture_names:
                    cap_val = (effective_value if effective_value is not None
                               else self._text[start_pos:pos])
                    new_caps = {**parent_caps, rname: cap_val}
                for link in ret_links:
                    self._find_completion(link, new_stack, new_ch, pos,
                                          new_caps, results, visited)
            else:
                action_result = caps.get('__result__', None)
                effective_value = action_result
                if effective_value is None and len(children) == 1 \
                        and children[0].value is not None:
                    effective_value = children[0].value
                r = ParseNode(node.rule, self._text[:pos],
                              list(children), 0, pos,
                              value=effective_value)
                results.append(r)

    # ---- helpers -----------------------------------------------------

    @staticmethod
    def _char_matches(node, ch):
        if node.type == NT.MATCH_CHAR:
            return ch == node.value
        if node.type == NT.MATCH_CLASS:
            return node.value.matches(ch)
        if node.type == NT.MATCH_ANY:
            return True
        return False

    @staticmethod
    def _dedup(cursors):
        # First-seen wins.  Combined with link-order traversal in
        # _expand, this implements ordered choice: the cursor derived
        # from link[0] of any given fork arrives first, so it's kept.
        seen = {}
        for cursor in cursors:
            node, stack, children, caps = cursor
            sk = tuple((r, rl, sp) for r, rl, sp, *_ in stack)
            ck = tuple(sorted(caps.items())) if caps else ()
            key = (node.id, sk, ck)
            if key not in seen:
                seen[key] = cursor
        return list(seen.values())


# ========================== Bootstrap Parser ==========================

class UnifiedBootstrap:
    """Recursive-descent parser for unified EEBNF grammars.

    Unified syntax:
        start <rule_name>
        name = alternatives
        alternatives = sequence ('|' sequence)*
        element = atom modifier?
        atom = 'string' | [charclass] | . | name | name:=(alts)
             | '(' alternatives ')' | &atom | !atom
    """

    _LEXER_RULES = [
        ('COMMENT',    r'#[^\n]*'),
        ('NEWLINE',    r'\n'),
        ('WS',         r'[ \t\r]+'),
        ('START_KW',   r'start\b'),
        ('STARQ',      r'\*\?'),
        ('PLUSQ',      r'\+\?'),
        ('QQMARK',     r'\?\?'),
        ('STAR',       r'\*'),
        ('PLUS',       r'\+'),
        ('QMARK',      r'\?'),
        ('OR',         r'\|'),
        ('CAPTURE',    r':='),
        ('EQUALS',     r'='),
        ('LPAREN',     r'\('),
        ('RPAREN',     r'\)'),
        # ACTION must be matched before LBRACE so `{{` is recognised as
        # an action-block opener rather than two `{` tokens (one of
        # which would try to start a `{n,m}` repetition modifier).
        ('ACTION',     r'\{\{[\s\S]*?\}\}'),
        ('LBRACE',     r'\{'),
        ('RBRACE',     r'\}'),
        ('COMMA',      r','),
        ('NUMBER',     r'\d+'),
        ('DOT',        r'\.'),
        ('MINUS',      r'-'),
        ('AND_OP',     r'&'),
        ('NOT_OP',     r'!'),
        ('SKIP_KW',    r'@skip\b'),
        ('LONGEST_KW', r'@longest\b'),
        ('KW_KW',      r'@keyword\b'),
        ('ATOMIC_KW',  r'@atomic\b'),
        ('MODE_KW',    r'@mode\b'),
        ('LEFT_KW',    r'@left\b'),
        ('RIGHT_KW',   r'@right\b'),
        ('NONASSOC_KW',r'@nonassoc\b'),
        ('REGEX',      r'/(?:\\.|[^/\\])*/[a-z]*'),
        ('CHARCLASS',  r'\[(?:\\.|[^\]])*\]'),
        ('STRING',     r"'(?:\\.|[^'\\])*'|\"(?:\\.|[^\"\\])*\""),
        ('NAME',       r'[a-zA-Z_][a-zA-Z_0-9]*'),
    ]

    def __init__(self, text):
        self.tokens = self._tokenize(text)
        self.pos = 0
        self._skip_nl()

    # ---- mini lexer (just for bootstrapping the grammar files) -------

    def _tokenize(self, text):
        compiled = [(n, re.compile(p)) for n, p in self._LEXER_RULES]
        ignore = {'WS', 'COMMENT'}
        tokens, pos, line, col = [], 0, 1, 1
        while pos < len(text):
            for name, pat in compiled:
                m = pat.match(text, pos)
                if m:
                    val = m.group()
                    if name not in ignore:
                        tokens.append((name, val, line, col))
                    for ch in val:
                        if ch == '\n': line += 1; col = 1
                        else: col += 1
                    pos = m.end(); break
            else:
                raise SyntaxError(
                    f"Unexpected {text[pos]!r} at line {line}, col {col}")
        tokens.append(('EOF', '', line, col))
        return tokens

    # ---- token helpers -----------------------------------------------

    def _peek(self):   return self.tokens[self.pos]
    def _advance(self):
        t = self.tokens[self.pos]; self.pos += 1; return t
    def _at(self, *types): return self._peek()[0] in types
    def _skip_nl(self):
        while self._at('NEWLINE'): self._advance()
    def _expect(self, tp):
        t = self._peek()
        if t[0] != tp:
            raise SyntaxError(
                f"Expected {tp}, got {t[0]} {t[1]!r} at line {t[2]}")
        return self._advance()

    # ---- grammar rules -----------------------------------------------

    def parse(self):
        rules = {}; start = None
        skip_rules = set()
        longest_rules = set()
        atomic_rules = set()
        keywords = set()
        rule_modes = {}   # non-skip rule name -> mode name
        skip_modes = {}   # skip rule name -> mode name
        # Precedence declarations: list of (assoc, [op_literals]) in the
        # order they appeared — earliest = lowest precedence, matching
        # bison/yacc convention.  Used by _apply_precedence at build
        # time to rewrite ambiguous binary-op rules into a hierarchy.
        precedence = []
        while not self._at('EOF'):
            self._skip_nl()
            if self._at('EOF'): break
            if self._at('START_KW'):
                self._advance(); start = self._expect('NAME')[1]
                self._skip_nl()
            elif self._at('KW_KW'):
                # Top-level keyword declaration: @keyword 'a' 'b' 'c'
                self._advance()
                got_any = False
                while self._at('STRING'):
                    s = self._advance()[1]
                    keywords.add(_unescape(s[1:-1]))
                    got_any = True
                if not got_any:
                    t = self._peek()
                    raise SyntaxError(
                        f"@keyword at top level expects at least one "
                        f"string literal at line {t[2]}")
                self._skip_nl()
            elif self._at('LEFT_KW', 'RIGHT_KW', 'NONASSOC_KW'):
                # Top-level precedence declaration:
                #   @left 'op1' 'op2' ...           # string literals
                #   @left rulename                  # rule reference
                #   @left '+' rulename '*'          # mixed
                t = self._advance()
                assoc = {'LEFT_KW': 'left',
                         'RIGHT_KW': 'right',
                         'NONASSOC_KW': 'nonassoc'}[t[0]]
                ops = []  # list of ('string', value) or ('name', value)
                while self._at('STRING') or self._at('NAME'):
                    if self._at('STRING'):
                        s = self._advance()[1]
                        ops.append(('string', _unescape(s[1:-1])))
                    else:
                        n = self._advance()[1]
                        ops.append(('name', n))
                if not ops:
                    tt = self._peek()
                    raise SyntaxError(
                        f"@{assoc} expects at least one operator (string "
                        f"literal or rule/token name) at line {tt[2]}")
                precedence.append((assoc, ops))
                self._skip_nl()
            elif self._at('NAME'):
                (name, alts, is_skip, is_longest, is_atomic,
                 mode_name) = self._parse_rule()
                rules[name] = alts
                if is_skip:
                    skip_rules.add(name)
                    skip_modes[name] = mode_name or 'default'
                else:
                    rule_modes[name] = mode_name or 'default'
                if is_longest:
                    longest_rules.add(name)
                if is_atomic:
                    atomic_rules.add(name)
                self._skip_nl()
            else:
                t = self._peek()
                raise SyntaxError(f"Unexpected {t[0]} at line {t[2]}")
        if not start and rules:
            start = next(iter(rules))
        return {'start': start, 'rules': rules,
                'skip_rules': skip_rules,
                'longest_rules': longest_rules,
                'atomic_rules': atomic_rules,
                'keywords': keywords,
                'rule_modes': rule_modes,
                'skip_modes': skip_modes,
                'precedence': precedence}

    def _parse_rule(self):
        name = self._expect('NAME')[1]
        self._expect('EQUALS')
        alts = self._parse_alternatives()
        is_skip = False
        is_longest = False
        is_atomic = False
        mode_name = None
        # Accept @skip, @longest, @atomic, @mode <name> in any order.
        while self._at('SKIP_KW') or self._at('LONGEST_KW') \
                or self._at('ATOMIC_KW') or self._at('MODE_KW'):
            t = self._advance()
            if t[0] == 'SKIP_KW':
                is_skip = True
            elif t[0] == 'LONGEST_KW':
                is_longest = True
            elif t[0] == 'ATOMIC_KW':
                is_atomic = True
            else:  # MODE_KW
                nm = self._expect('NAME')
                mode_name = nm[1]
        return name, alts, is_skip, is_longest, is_atomic, mode_name

    def _parse_alternatives(self):
        alts = [self._parse_sequence()]
        while True:
            self._skip_nl()
            if self._at('OR'):
                self._advance(); alts.append(self._parse_sequence())
            else: break
        return alts

    def _parse_sequence(self):
        elems = []
        while self._at('STRING', 'NAME', 'START_KW', 'LPAREN',
                        'DOT', 'CHARCLASS', 'REGEX', 'AND_OP', 'NOT_OP',
                        'KW_KW'):
            elems.append(self._parse_element())
        # Optional trailing semantic action: {{ python_expr }}.  The
        # action is encoded as a trailing pseudo-element that the graph
        # builder turns into an NT.ACTION node.
        if self._at('ACTION'):
            t = self._advance()
            # Strip outer {{ and }}, keep interior verbatim.
            code = t[1][2:-2].strip()
            elems.append({'type': 'action', 'code': code})
        return elems

    def _parse_element(self):
        elem = self._parse_atom()
        mod = self._parse_modifier()
        if mod is not None:
            elem['modifier'] = mod
        # EEBNF subtraction: `A - B` matches A but rejects if B matches
        # the same span.  Binds looser than modifiers (so `A* - B` means
        # `(A*) - B`) and tighter than juxtaposition / `|`.
        if self._at('MINUS'):
            self._advance()
            right = self._parse_atom()
            rmod = self._parse_modifier()
            if rmod is not None:
                right['modifier'] = rmod
            elem = {'type': 'subtract', 'left': elem, 'right': right}
        return elem

    def _parse_atom(self):
        t = self._peek()
        if t[0] == 'STRING':
            self._advance()
            return {'type': 'string', 'value': _unescape(t[1][1:-1])}
        if t[0] == 'CHARCLASS':
            self._advance()
            return {'type': 'charclass',
                    'charclass': _parse_charclass(t[1][1:-1])}
        if t[0] == 'REGEX':
            self._advance()
            pattern, flags = _split_regex(t[1])
            return _compile_regex(pattern, flags)
        if t[0] == 'KW_KW':
            # Inline @keyword wraps the next atom (must be a string) with a
            # "!wordchar" trailing check so the keyword only matches as a
            # complete word, not a prefix of a longer identifier.
            self._advance()
            inner = self._parse_atom()
            if inner.get('type') != 'string':
                raise SyntaxError(
                    f"@keyword must prefix a string literal (got {inner!r})"
                )
            return _wrap_keyword(inner['value'])
        if t[0] == 'DOT':
            self._advance()
            return {'type': 'any'}
        if t[0] in ('NAME', 'START_KW'):
            self._advance()
            name = t[1]
            # Named capture: name:=(alternatives)
            if self._at('CAPTURE'):
                self._advance()
                self._expect('LPAREN')
                alts = self._parse_alternatives()
                self._expect('RPAREN')
                return {'type': 'capture', 'name': name, 'alternatives': alts}
            return {'type': 'rule', 'value': name}
        if t[0] == 'LPAREN':
            self._advance()
            alts = self._parse_alternatives()
            self._expect('RPAREN')
            return {'type': 'group', 'alternatives': alts}
        if t[0] in ('AND_OP', 'NOT_OP'):
            op_type = 'and' if t[0] == 'AND_OP' else 'not'
            self._advance()
            inner = self._parse_atom()
            # Wrap in alternatives form
            if inner.get('type') == 'group':
                alts = inner['alternatives']
            else:
                alts = [[inner]]
            return {'type': op_type, 'alternatives': alts}
        raise SyntaxError(
            f"Expected atom, got {t[0]} {t[1]!r} at line {t[2]}")

    def _parse_modifier(self):
        t = self._peek()
        if t[0] == 'STARQ':  self._advance(); return '*?'
        if t[0] == 'STAR':   self._advance(); return '*'
        if t[0] == 'PLUSQ':  self._advance(); return '+?'
        if t[0] == 'PLUS':   self._advance(); return '+'
        if t[0] == 'QQMARK': self._advance(); return '??'
        if t[0] == 'QMARK':  self._advance(); return '?'
        if t[0] == 'LBRACE':
            self._expect('LBRACE')
            # Forms supported:
            #   {n}    exactly n
            #   {n,}   at least n (unbounded upper; max = -1)
            #   {n,m}  between n and m
            #   {,m}   at most m (min = 0)
            if self._at('COMMA'):
                self._advance()
                lo = 0
                hi = int(self._expect('NUMBER')[1])
                self._expect('RBRACE')
                mod = {'min': lo, 'max': hi, 'greedy': True}
            else:
                lo = int(self._expect('NUMBER')[1])
                if self._at('COMMA'):
                    self._advance()
                    if self._at('NUMBER'):
                        hi = int(self._advance()[1])
                    else:
                        hi = -1
                    self._expect('RBRACE')
                    mod = {'min': lo, 'max': hi, 'greedy': True}
                else:
                    self._expect('RBRACE')
                    mod = {'min': lo, 'max': lo, 'greedy': True}
            if self._at('QMARK'):
                self._advance(); mod['greedy'] = False
            return mod
        return None


# ========================== EBNF Bootstrap (ISO 14977) ==================

class EBNFBootstrap:
    """Recursive-descent parser for ISO 14977 EBNF grammars.

    Supported syntax:
        syntax_rule       = meta_identifier '=' definitions_list ';'
        definitions_list  = single_definition ( '|' single_definition )*
        single_definition = syntactic_term ( ',' syntactic_term )*
        syntactic_term    = syntactic_factor ( '-' syntactic_factor )?
        syntactic_factor  = [ integer '*' ] syntactic_primary
        syntactic_primary = terminal_string | meta_identifier
                          | '[' definitions_list ']'     (optional)
                          | '{' definitions_list '}'     (zero-or-more)
                          | '(' definitions_list ')'     (grouping)
        terminal_string   = "'" ... "'" | '"' ... '"'
        meta_identifier   = letter ( letter | digit | '_' | '-' )*
        comments          = '(*' ... '*)'

    Rejected (SyntaxError):
        '?special sequence?' — implementation-defined, not interpretable
        All EEBNF extensions (regex, captures, predicates, actions, @*)
    """

    _LEXER_RULES = [
        ('COMMENT', r'\(\*(?:[^*]|\*(?!\)))*\*\)'),
        ('WS',      r'[ \t\r\n]+'),
        ('LBRACE',  r'\{'),
        ('RBRACE',  r'\}'),
        ('LBRACK',  r'\['),
        ('RBRACK',  r'\]'),
        ('LPAREN',  r'\('),
        ('RPAREN',  r'\)'),
        ('SEMI',    r';'),
        ('EQUALS',  r'='),
        ('COMMA',   r','),
        ('OR',      r'\|'),
        ('STAR',    r'\*'),
        ('MINUS',   r'-'),
        ('SPECIAL', r'\?[^?]*\?'),
        ('NUMBER',  r'\d+'),
        ('STRING',  r'"[^"]*"' + r"|'[^']*'"),
        ('NAME',    r'[a-zA-Z][a-zA-Z0-9_\-]*'),
    ]

    def __init__(self, text):
        self.tokens = self._tokenize(text)
        self.pos = 0

    def _tokenize(self, text):
        compiled = [(n, re.compile(p)) for n, p in self._LEXER_RULES]
        ignore = {'WS', 'COMMENT'}
        toks, p, line, col = [], 0, 1, 1
        while p < len(text):
            for name, pat in compiled:
                m = pat.match(text, p)
                if m:
                    val = m.group()
                    if name not in ignore:
                        toks.append((name, val, line, col))
                    for ch in val:
                        if ch == '\n': line += 1; col = 1
                        else:          col += 1
                    p = m.end()
                    break
            else:
                raise SyntaxError(
                    f"Unexpected {text[p]!r} at line {line}, col {col}")
        toks.append(('EOF', '', line, col))
        return toks

    def _peek(self):       return self.tokens[self.pos]
    def _peek_at(self, k): return self.tokens[self.pos + k]
    def _advance(self):
        t = self.tokens[self.pos]; self.pos += 1; return t
    def _at(self, *types): return self._peek()[0] in types
    def _expect(self, tp):
        t = self._peek()
        if t[0] != tp:
            raise SyntaxError(
                f"Expected {tp}, got {t[0]} {t[1]!r} at line {t[2]}")
        return self._advance()

    def parse(self):
        rules, order = {}, []
        while not self._at('EOF'):
            name = self._expect('NAME')[1]
            self._expect('EQUALS')
            alts = self._parse_definitions_list()
            self._expect('SEMI')
            rules[name] = alts
            order.append(name)
        start = order[0] if order else None
        return {
            'start': start,
            'rules': rules,
            'skip_rules': set(),
            'longest_rules': set(),
            'atomic_rules': set(),
            'keywords': set(),
            'rule_modes': {n: 'default' for n in rules},
            'skip_modes': {},
        }

    def _parse_definitions_list(self):
        alts = [self._parse_single_definition()]
        while self._at('OR'):
            self._advance()
            alts.append(self._parse_single_definition())
        return alts

    def _parse_single_definition(self):
        terms = [self._parse_syntactic_term()]
        while self._at('COMMA'):
            self._advance()
            terms.append(self._parse_syntactic_term())
        return terms

    def _parse_syntactic_term(self):
        factor = self._parse_syntactic_factor()
        if self._at('MINUS'):
            self._advance()
            right = self._parse_syntactic_factor()
            return {'type': 'subtract', 'left': factor, 'right': right}
        return factor

    def _parse_syntactic_factor(self):
        # N * primary — exactly N copies
        if self._at('NUMBER') and self._peek_at(1)[0] == 'STAR':
            n = int(self._advance()[1])
            self._advance()  # STAR
            primary = self._parse_syntactic_primary()
            primary = {'type': 'group', 'alternatives': [[primary]],
                       'modifier': {'min': n, 'max': n, 'greedy': True}}
            return primary
        return self._parse_syntactic_primary()

    def _parse_syntactic_primary(self):
        t = self._peek()
        if t[0] == 'STRING':
            self._advance()
            return {'type': 'string', 'value': t[1][1:-1]}
        if t[0] == 'NAME':
            self._advance()
            return {'type': 'rule', 'value': t[1]}
        if t[0] == 'LPAREN':
            self._advance()
            alts = self._parse_definitions_list()
            self._expect('RPAREN')
            return {'type': 'group', 'alternatives': alts}
        if t[0] == 'LBRACK':
            self._advance()
            alts = self._parse_definitions_list()
            self._expect('RBRACK')
            return {'type': 'group', 'alternatives': alts, 'modifier': '?'}
        if t[0] == 'LBRACE':
            self._advance()
            alts = self._parse_definitions_list()
            self._expect('RBRACE')
            return {'type': 'group', 'alternatives': alts, 'modifier': '*'}
        if t[0] == 'SPECIAL':
            raise SyntaxError(
                f"?special? sequences are implementation-defined and not "
                f"supported (line {t[2]})")
        raise SyntaxError(
            f"Expected primary, got {t[0]} {t[1]!r} at line {t[2]}")


# ========================== High-Level API ==========================

def load_grammar(text, ebnf=False):
    """Parse a grammar string and return a ScannerlessParser.

    By default the grammar is parsed as unified EEBNF (GPDA's extended
    EBNF with regex, predicates, captures, actions, @directives, etc).
    Pass ``ebnf=True`` to parse as ISO 14977 EBNF instead — a strict
    subset where `A - B` subtraction is supported but no EEBNF
    extensions are permitted."""
    if ebnf:
        grammar = EBNFBootstrap(text).parse()
    else:
        grammar = UnifiedBootstrap(text).parse()
    builder = GraphBuilder()
    (rules, start, lr_meta, capture_names, dfas,
     stripped_names) = builder.build(grammar)
    return ScannerlessParser(rules, start, lr_meta, capture_names, dfas,
                              stripped_names)


# ========================== Tests ==========================

if __name__ == '__main__':
    passed = failed = 0

    def check(name, cond):
        global passed, failed
        if cond: print(f"  PASS: {name}"); passed += 1
        else:    print(f"  FAIL: {name}"); failed += 1

    # ---- Test 1: literal string ----
    print("\n--- Test 1: literal string ---")
    p = load_grammar("start g\ng = 'hello'")
    tree = p.parse("hello")
    check("matches 'hello'", tree.name == 'g' and tree.text == 'hello')
    try:
        p.parse("hell"); check("rejects 'hell'", False)
    except SyntaxError:
        check("rejects 'hell'", True)

    # ---- Test 2: character class ----
    print("\n--- Test 2: character class ---")
    p = load_grammar("start d\nd = [0-9]+")
    tree = p.parse("42")
    check("matches '42'", tree.text == '42')
    try:
        p.parse("ab"); check("rejects 'ab'", False)
    except SyntaxError:
        check("rejects 'ab'", True)

    # ---- Test 3: negated character class ----
    print("\n--- Test 3: negated char class ---")
    p = load_grammar("start g\ng = [^0-9]+")
    tree = p.parse("abc")
    check("matches 'abc'", tree.text == 'abc')

    # ---- Test 4: dot (any character) ----
    print("\n--- Test 4: dot ---")
    p = load_grammar("start g\ng = .{3}")
    tree = p.parse("xyz")
    check("matches 'xyz'", tree.text == 'xyz')
    tree = p.parse("!@#")
    check("matches '!@#'", tree.text == '!@#')

    # ---- Test 5: alternatives ----
    print("\n--- Test 5: alternatives ---")
    p = load_grammar("start g\ng = 'yes' | 'no'")
    check("matches 'yes'", p.parse("yes").text == 'yes')
    check("matches 'no'",  p.parse("no").text == 'no')

    # ---- Test 6: nested rules ----
    print("\n--- Test 6: nested rules ---")
    p = load_grammar("""
        start expr
        expr = number '+' number
        number = [0-9]+
    """)
    tree = p.parse("12+34")
    check("matches '12+34'", tree.text == '12+34')
    check("two number children", len(tree.children) == 2)
    check("first number is '12'", tree.children[0].text == '12')
    check("second number is '34'", tree.children[1].text == '34')
    print(tree.pretty())

    # ---- Test 7: expression grammar with whitespace ----
    print("\n--- Test 7: expression grammar ---")
    p = load_grammar("""
        start expr
        ws = [ \t]*
        number = [0-9]+
        expr = ws term (ws '+' ws term)*
        term = ws factor (ws '*' ws factor)*
        factor = ws number | ws '(' expr ws ')'
    """)
    tree = p.parse("1+2*3")
    check("parses '1+2*3'", tree.name == 'expr')
    print(tree.pretty())

    tree = p.parse("(1 + 2) * 3")
    check("parses '(1 + 2) * 3'", tree.name == 'expr')
    print(tree.pretty())

    # ---- Test 8: star/plus/optional on characters ----
    print("\n--- Test 8: star/plus/optional ---")
    p = load_grammar("start g\ng = 'a'* 'b'")
    check("'b' matches a*b",   p.parse("b").text == 'b')
    check("'ab' matches a*b",  p.parse("ab").text == 'ab')
    check("'aaab' matches a*b", p.parse("aaab").text == 'aaab')

    p = load_grammar("start g\ng = 'a'+ 'b'")
    try:
        p.parse("b"); check("a+b rejects 'b'", False)
    except SyntaxError:
        check("a+b rejects 'b'", True)
    check("'ab' matches a+b", p.parse("ab").text == 'ab')

    p = load_grammar("start g\ng = 'a'? 'b'")
    check("'b' matches a?b",  p.parse("b").text == 'b')
    check("'ab' matches a?b", p.parse("ab").text == 'ab')

    # ---- Test 9: escape sequences ----
    print("\n--- Test 9: escape sequences ---")
    p = load_grammar(r"start g" + "\n" + r"g = 'hello\nworld'")
    tree = p.parse("hello\nworld")
    check("matches with newline", tree.text == "hello\nworld")

    p = load_grammar("start g\ng = [\\t]+")
    tree = p.parse("\t\t")
    check("matches tabs", tree.text == "\t\t")

    # ---- Test 10: left recursion (direct) ----
    print("\n--- Test 10: left recursion ---")
    p = load_grammar("""
        start expr
        number = [0-9]+
        expr = expr '+' term | term
        term = number
    """)
    tree = p.parse("1+2+3")
    check("lr: parses '1+2+3'", tree.name == 'expr')
    check("lr: left-assoc outer", tree.children[0].name == 'expr')
    check("lr: left-assoc inner", tree.children[0].children[0].name == 'expr')
    print(tree.pretty())

    # ---- Test 11: realistic: simple JSON-like values ----
    print("\n--- Test 11: JSON-like parser ---")
    p = load_grammar(r"""
        start value
        ws = [ \t\n]*
        value = ws (string | number | array | object | bool | null) ws
        string = '"' strchar* '"'
        strchar = [^"\\] | '\\' .
        number = '-'? [0-9]+ ('.' [0-9]+)?
        array = '[' ws (value (',' value)*)? ws ']'
        object = '{' ws (pair (',' pair)*)? ws '}'
        pair = ws string ws ':' value
        bool = 'true' | 'false'
        null = 'null'
    """)
    tree = p.parse('{"a": [1, 2, 3], "b": true}')
    check("JSON: parses object", tree.name == 'value')
    print(tree.pretty())

    tree = p.parse('[1, "hello", null, false]')
    check("JSON: parses array", tree.name == 'value')

    tree = p.parse('"hello \\"world\\""')
    check("JSON: parses escaped string", tree.name == 'value')

    # ---- Test 12: NOT predicate (!) ----
    print("\n--- Test 12: NOT predicate ---")
    # !'b' succeeds when next char is NOT 'b'
    p = load_grammar("start g\ng = 'a' !'b' .")
    check("!: 'ac' passes (c is not b)", p.parse("ac").text == 'ac')
    try:
        p.parse("ab"); check("!: 'ab' rejected", False)
    except SyntaxError:
        check("!: 'ab' rejected", True)

    # Match any char except 'x', repeated
    p = load_grammar("start g\ng = (!'x' .)+")
    check("!: 'abc' all non-x", p.parse("abc").text == 'abc')
    try:
        p.parse("abxc"); check("!: 'abxc' rejected at x", False)
    except SyntaxError:
        check("!: 'abxc' rejected at x", True)

    # NOT at end of input: !'z' succeeds when nothing follows
    p = load_grammar("start g\ng = 'a' !'z'")
    check("!: 'a' at EOF (not z)", p.parse("a").text == 'a')

    # ---- Test 13: AND predicate (&) ----
    print("\n--- Test 13: AND predicate ---")
    # &'a' succeeds only when next char IS 'a', but doesn't consume it
    p = load_grammar("start g\ng = &'a' .")
    check("&: 'a' passes", p.parse("a").text == 'a')
    try:
        p.parse("b"); check("&: 'b' rejected", False)
    except SyntaxError:
        check("&: 'b' rejected", True)

    # AND with multi-char lookahead
    p = load_grammar("start g\ng = &('ab') ..")
    check("&: 'ab' passes 2-char lookahead", p.parse("ab").text == 'ab')
    try:
        p.parse("ac"); check("&: 'ac' rejected", False)
    except SyntaxError:
        check("&: 'ac' rejected", True)

    # ---- Test 14: nested predicates ----
    print("\n--- Test 14: nested predicates ---")
    # Double negation: !!'a' is equivalent to &'a'
    p = load_grammar("start g\ng = !!'a' .")
    check("!!: double neg 'a' passes", p.parse("a").text == 'a')
    try:
        p.parse("b"); check("!!: 'b' rejected", False)
    except SyntaxError:
        check("!!: 'b' rejected", True)

    # NOT with grouped expression
    p = load_grammar("start g\ng = !('ab') . .")
    check("!(ab): 'xy' passes", p.parse("xy").text == 'xy')
    try:
        p.parse("ab"); check("!(ab): 'ab' rejected", False)
    except SyntaxError:
        check("!(ab): 'ab' rejected", True)
    check("!(ab): 'ac' passes (not full 'ab')", p.parse("ac").text == 'ac')

    # ---- Test 15: predicates with rules ----
    print("\n--- Test 15: predicates referencing rules ---")
    p = load_grammar("""
        start id
        keyword = 'if' | 'do' | 'fn'
        id_char = [a-zA-Z_]
        id = !(keyword !id_char) id_char+
    """)
    check("id: 'foo' is not a keyword", p.parse("foo").text == 'foo')
    check("id: 'iff' is not keyword 'if'", p.parse("iff").text == 'iff')
    try:
        p.parse("if"); check("id: 'if' rejected (keyword)", False)
    except SyntaxError:
        check("id: 'if' rejected (keyword)", True)
    try:
        p.parse("do"); check("id: 'do' rejected (keyword)", False)
    except SyntaxError:
        check("id: 'do' rejected (keyword)", True)
    check("id: 'done' is not keyword 'do'", p.parse("done").text == 'done')

    # ---- Test 16: named captures ----
    print("\n--- Test 16: named captures ---")
    p = load_grammar("""
        start g
        g = greeting:=([a-zA-Z]+) ' ' name:=([a-zA-Z]+)
    """)
    tree = p.parse("hello world")
    check("capture: root is g", tree.name == 'g')
    check("capture: 2 children", len(tree.children) == 2)
    check("capture: first is 'greeting'", tree.children[0].name == 'greeting')
    check("capture: greeting text", tree.children[0].text == 'hello')
    check("capture: second is 'name'", tree.children[1].name == 'name')
    check("capture: name text", tree.children[1].text == 'world')
    print(tree.pretty())

    # Capture with modifier
    p = load_grammar("""
        start g
        g = items:=([a-z]+) (',' items:=([a-z]+))*
    """)
    tree = p.parse("foo,bar,baz")
    check("capture+mod: parses csv", tree.text == 'foo,bar,baz')
    # Should have 3 items children (all named 'items')
    items_children = [c for c in tree.children if c.name == 'items']
    check("capture+mod: 3 items", len(items_children) == 3)
    print(tree.pretty())

    # ---- Test 17: backreferences — quote matching ----
    print("\n--- Test 17: backreferences ---")
    p = load_grammar(r"""
        start string
        string = q:=(['"]) (!q .)* q
    """)
    tree = p.parse("'hello'")
    check("backref: single-quoted", tree.text == "'hello'")
    tree = p.parse('"hello"')
    check("backref: double-quoted", tree.text == '"hello"')
    try:
        p.parse("'hello\"")
        check("backref: mismatched rejected", False)
    except SyntaxError:
        check("backref: mismatched rejected", True)
    try:
        p.parse('"hello\'')
        check("backref: other mismatch rejected", False)
    except SyntaxError:
        check("backref: other mismatch rejected", True)
    print(tree.pretty())

    # ---- Test 18: backreference with multi-char capture ----
    print("\n--- Test 18: multi-char backref ---")
    p = load_grammar(r"""
        start heredoc
        heredoc = '<<' tag:=([a-zA-Z]+) '\n' (!('\n' tag) .)* '\n' tag
    """)
    tree = p.parse("<<END\nhello world\nEND")
    check("heredoc: parses", tree.text == "<<END\nhello world\nEND")
    tree = p.parse("<<FOO\nline1\nline2\nFOO")
    check("heredoc: different tag", tree.text == "<<FOO\nline1\nline2\nFOO")
    try:
        p.parse("<<FOO\nstuff\nBAR")
        check("heredoc: wrong closing tag", False)
    except SyntaxError:
        check("heredoc: wrong closing tag", True)
    print(tree.pretty())

    # ---- Test 19b: backref in predicate ----
    print("\n--- Test 19b: backref in predicate ---")
    p = load_grammar(r"""
        start g
        g = d:=([0-9]) [a-z]+ d
    """)
    tree = p.parse("3abc3")
    check("backref pred: '3abc3' matches", tree.text == '3abc3')
    try:
        p.parse("3abc4")
        check("backref pred: '3abc4' rejected", False)
    except SyntaxError:
        check("backref pred: '3abc4' rejected", True)

    # ---- Test 19: auto-ws insertion ----
    print("\n--- Test 19: auto-ws insertion ---")
    # Defining `_` enables auto-insertion between elements
    p = load_grammar(r"""
        start expr
        _ = [ \t]*
        expr = term '+' term
        term = [0-9]+
    """)
    tree = p.parse("1+2")
    check("auto-ws: no whitespace works", tree.name == 'expr')
    tree = p.parse("1 + 2")
    check("auto-ws: single spaces work", tree.name == 'expr')
    tree = p.parse("1   +  2")
    check("auto-ws: multiple spaces work", tree.name == 'expr')
    tree = p.parse("1\t+\t\t2")
    check("auto-ws: tabs work", tree.name == 'expr')
    print(tree.pretty())

    # ---- Test 20: atomic rules (UPPERCASE) ----
    print("\n--- Test 20: atomic rules ---")
    # Uppercase names are atomic — no ws insertion inside them
    p = load_grammar(r"""
        start prog
        _ = [ \t]*
        IDENT = [a-zA-Z_] [a-zA-Z_0-9]*
        prog = IDENT '=' IDENT
    """)
    tree = p.parse("foo = bar")
    check("atomic: IDENT works with ws between", tree.name == 'prog')
    # Whitespace inside IDENT should fail:
    try:
        p.parse("foo b = bar")
        check("atomic: ws inside IDENT rejected", False)
    except SyntaxError:
        check("atomic: ws inside IDENT rejected", True)

    # ---- Test 21: no auto-ws when `_` not defined ----
    print("\n--- Test 21: no _ = no auto-ws ---")
    # Without `_`, whitespace matters — grammar must handle it explicitly
    p = load_grammar(r"""
        start expr
        expr = [0-9]+ '+' [0-9]+
    """)
    tree = p.parse("1+2")
    check("no _: no-ws input works", tree.name == 'expr')
    try:
        p.parse("1 + 2")
        check("no _: ws input rejected", False)
    except SyntaxError:
        check("no _: ws input rejected", True)

    # ---- Test 22: explicit _ reference doesn't duplicate ----
    print("\n--- Test 22: explicit _ references ---")
    p = load_grammar(r"""
        start stmt
        _ = [ \t]*
        stmt = 'return' _ value
        value = [0-9]+
    """)
    tree = p.parse("return 42")
    check("explicit _: 'return 42' works", tree.name == 'stmt')
    tree = p.parse("return42")
    check("explicit _: 'return42' also works (zero ws)",
          tree.name == 'stmt')

    # ---- Test 23: auto-ws + left recursion ----
    print("\n--- Test 23: auto-ws with left recursion ---")
    p = load_grammar(r"""
        start expr
        _ = [ \t]*
        expr = expr '+' term | term
        term = [0-9]+
    """)
    tree = p.parse("1 + 2 + 3")
    check("lr+ws: parses with spaces", tree.name == 'expr')
    # Left-associative
    check("lr+ws: outer left is expr",
          tree.children[0].name == 'expr')
    check("lr+ws: inner left is expr",
          tree.children[0].children[0].name == 'expr')
    print(tree.pretty())

    # ---- Test 24: end-to-end calculator-style grammar ----
    print("\n--- Test 24: end-to-end calculator ---")
    p = load_grammar(r"""
        start expr
        _ = [ \t\n]*
        NUMBER = [0-9]+
        expr = expr '+' term | expr '-' term | term
        term = term '*' factor | term '/' factor | factor
        factor = NUMBER | '(' expr ')'
    """)
    tree = p.parse("1 + 2 * 3")
    check("calc: '1 + 2 * 3' parses", tree.name == 'expr')
    tree = p.parse("(1 + 2) * 3")
    check("calc: '(1 + 2) * 3' parses", tree.name == 'expr')
    # Leading/trailing ws is NOT auto-handled (only inter-element ws is)
    try:
        p.parse("  10 - 2 / 5  ")
        check("calc: leading ws fails without wrapping", False)
    except SyntaxError:
        check("calc: leading ws fails without wrapping", True)

    # To allow leading/trailing ws, wrap the start rule manually:
    p = load_grammar(r"""
        start prog
        _ = [ \t\n]*
        NUMBER = [0-9]+
        prog = _ expr _
        expr = expr '+' term | term
        term = NUMBER
    """)
    tree = p.parse("  12 + 34  ")
    check("calc: leading/trailing ws with wrapping",
          tree.name == 'prog')
    print(tree.pretty())

    # ---- Test 24: inline /regex/ literals ----
    print("\n--- Test 24: inline /regex/ ---")
    p = load_grammar(r"""
        start expr
        expr = /[0-9]+/
    """)
    check("regex: /[0-9]+/ matches '123'", p.parse('123').text == '123')

    # Shorthand classes
    p = load_grammar(r"""
        start ident
        ident = /\w+/
    """)
    check("regex: \\w+ matches 'foo_bar42'",
          p.parse('foo_bar42').text == 'foo_bar42')

    p = load_grammar(r"""
        start nd
        nd = /\D+/
    """)
    check("regex: \\D+ matches 'abc!'", p.parse('abc!').text == 'abc!')

    p = load_grammar(r"""
        start gap
        gap = /\s+/
    """)
    check("regex: \\s+ matches ' \\t\\n'",
          p.parse(' \t\n').text == ' \t\n')

    # Alternation, grouping, quantifiers
    p = load_grammar(r"""
        start hex
        hex = /(0x)?[0-9a-fA-F]{2,8}/
    """)
    check("regex: hex '0xff'", p.parse('0xff').text == '0xff')
    check("regex: hex 'cafebabe'", p.parse('cafebabe').text == 'cafebabe')

    # Case-insensitive flag
    p = load_grammar(r"""
        start kw
        kw = /hello/i
    """)
    check("regex /i: Hello", p.parse('Hello').text == 'Hello')
    check("regex /i: HELLO", p.parse('HELLO').text == 'HELLO')
    check("regex /i: hElLo", p.parse('hElLo').text == 'hElLo')

    # Escape sequences
    p = load_grammar(r"""
        start nl
        nl = /\n+/
    """)
    check("regex: \\n+", p.parse('\n\n\n').text == '\n\n\n')

    # /regex/ mixed with other atoms
    p = load_grammar(r"""
        start expr
        NUMBER = /\d+/
        expr = NUMBER /\+/ NUMBER
    """)
    tree = p.parse("12+34")
    check("regex mixed: parses '12+34'", tree.name == 'expr')

    # Shorthand classes inside [...] (not just /regex/)
    p = load_grammar(r"""
        start IDENT
        IDENT = [a-zA-Z_][\w]*
    """)
    check("[\\w] mixed class: 'foo_99' matches",
          p.parse('foo_99').text == 'foo_99')

    # ---- Test 25: @skip marker ----
    print("\n--- Test 25: @skip marker ---")
    # Single @skip rule (one-or-more whitespace)
    p = load_grammar(r"""
        start expr
        ws = /\s+/ @skip
        NUMBER = /\d+/
        expr = NUMBER /\+/ NUMBER
    """)
    check("@skip: '1+2' no ws", p.parse('1+2').name == 'expr')
    check("@skip: '1 + 2' one ws", p.parse('1 + 2').name == 'expr')
    check("@skip: '1  \\t+\\n2' mixed", p.parse('1  \t+\n2').name == 'expr')

    # Multiple @skip rules (ws + comments)
    p = load_grammar(r"""
        start expr
        ws  = /[ \t\n]+/ @skip
        cmt = /#[^\n]*/ @skip
        NUMBER = /\d+/
        expr = NUMBER /\+/ NUMBER
    """)
    check("@skip multi: ws only", p.parse('1 + 2').name == 'expr')
    check("@skip multi: cmt only", p.parse('1#c\n+2').name == 'expr')
    check("@skip multi: ws+cmt mixed",
          p.parse('1 # one\n + # two\n 2').name == 'expr')

    # Explicit reference to a @skip rule: still required at that site
    p = load_grammar(r"""
        start prog
        ws = /\s+/ @skip
        LET   = /let/
        IDENT = /[a-zA-Z_][a-zA-Z_0-9]*/
        prog = LET ws IDENT
    """)
    check("@skip explicit: 'let foo' ok", p.parse('let foo').name == 'prog')
    try:
        p.parse('letfoo'); check("@skip explicit: 'letfoo' rejected", False)
    except SyntaxError:
        check("@skip explicit: 'letfoo' rejected", True)

    # @skip coexists with _ rule
    p = load_grammar(r"""
        start expr
        _   = /[ \t]*/
        cmt = /#[^\n]*\n/ @skip
        NUMBER = /\d+/
        expr = NUMBER /\+/ NUMBER
    """)
    check("@skip+_: '1+2'", p.parse('1+2').name == 'expr')
    check("@skip+_: '1 + 2'", p.parse('1 + 2').name == 'expr')
    check("@skip+_: '1 # cmt\\n+ 2'",
          p.parse('1 # cmt\n+ 2').name == 'expr')

    # ---- Test 26: DFA compilation — regularity analysis ----
    print("\n--- Test 26: DFA compilation ---")

    # Simple regex-only rules should compile to DFAs.
    p = load_grammar(r"""
        start main
        NUMBER = /\d+/
        IDENT  = /[a-zA-Z_]\w*/
        STR    = /"[^"]*"/
        main = NUMBER | IDENT | STR
    """)
    check("DFA: NUMBER is regular",  'NUMBER' in p.dfas)
    check("DFA: IDENT is regular",   'IDENT'  in p.dfas)
    check("DFA: STR is regular",     'STR'    in p.dfas)
    check("DFA: main (union) is regular", 'main' in p.dfas)

    # Accept positions of NUMBER on '12345'
    acc = p.dfas['NUMBER'].match_all('12345', 0)
    check("DFA NUMBER: accepts at 1..5", acc == [1, 2, 3, 4, 5])
    # No match — empty prefix not accepting (state 0 not accepting)
    check("DFA NUMBER: 'abc' rejects", p.dfas['NUMBER'].match_all('abc', 0) == [])
    # Quoted-string only accepts at the closing quote
    acc = p.dfas['STR'].match_all('"foo"rest', 0)
    check("DFA STR: only accepts at close quote", acc == [5])

    # Rules that use predicates / captures / backrefs are NOT regular.
    p = load_grammar(r"""
        start g
        g = &'x' 'y'
    """)
    check("predicate makes rule non-regular", 'g' not in p.dfas)

    p = load_grammar(r"""
        start g
        g = q:=(['"]) (!q .)* q
    """)
    check("backref makes rule non-regular", 'g' not in p.dfas)

    p = load_grammar(r"""
        start g
        g = name:=([a-z]+) '!'
    """)
    check("capture makes rule non-regular", 'g' not in p.dfas)

    # Self-recursive rule is not regular (regular languages are acyclic)
    p = load_grammar(r"""
        start list
        list = 'x' list | 'x'
    """)
    check("recursive rule non-regular", 'list' not in p.dfas)

    # A rule referencing a non-regular rule is itself non-regular.
    p = load_grammar(r"""
        start wrapper
        name = a:=([a-z]+)
        wrapper = name '!'
    """)
    check("rule referencing non-regular is non-regular",
          'wrapper' not in p.dfas)

    # Case-insensitive /regex/ compiles correctly via DFA too.
    p = load_grammar(r"""
        start kw
        kw = /hello/i
    """)
    dfa = p.dfas['kw']
    check("DFA /i: 'Hello' matches", dfa.match_all('Hello') == [5])
    check("DFA /i: 'HELLO' matches", dfa.match_all('HELLO') == [5])
    check("DFA /i: 'xello' rejects", dfa.match_all('xello') == [])

    # Quantified rule with bounded count
    p = load_grammar(r"""
        start hex
        hex = /[0-9a-fA-F]{2,4}/
    """)
    dfa = p.dfas['hex']
    # After 2 chars → accept, after 3 → accept, after 4 → accept, after 5 → no transition
    acc = dfa.match_all('deadbeef', 0)
    check("DFA {2,4}: stops at 4", acc == [2, 3, 4])

    # ---- Test 27: @longest directive ----
    print("\n--- Test 27: @longest directive ---")
    p = load_grammar(r"""
        start main
        NUMBER = /\d+/ @longest
        main = NUMBER
    """)
    dfa = p.dfas['NUMBER']
    check("@longest: DFA flag set", dfa.longest)
    check("@longest: only longest accept reported",
          dfa.match_all('12345') == [5])

    # Without @longest, all accepts returned
    p = load_grammar(r"""
        start main
        NUMBER = /\d+/
        main = NUMBER
    """)
    check("default: all accepts returned",
          p.dfas['NUMBER'].match_all('12345') == [1, 2, 3, 4, 5])

    # @longest on non-regular rule is a grammar error
    try:
        load_grammar(r"""
            start g
            g = &'x' 'y' @longest
        """)
        check("@longest on non-regular rejected", False)
    except SyntaxError:
        check("@longest on non-regular rejected", True)

    # ---- Test 28: @keyword keyword shorthand ----
    print("\n--- Test 28: @keyword ---")
    # Top-level @keyword
    p = load_grammar(r"""
        start prog
        @keyword 'class' 'if'
        _ = /\s*/ @longest
        IDENT = /[a-zA-Z_]\w*/ @longest
        stmt = 'class' IDENT | 'if' IDENT | IDENT
        prog = stmt (';' stmt)*
    """)
    check("@keyword: 'class foo' parses",
          p.parse('class foo').name == 'prog')
    check("@keyword: 'classroom' is an IDENT (not keyword + 'room')",
          p.parse('classroom').name == 'prog')
    # Semantic check: classroom should parse as a single IDENT stmt.
    # stmt here contains an IDENT with text 'classroom'.
    tree = p.parse('classroom')
    # Traverse: prog -> stmt -> IDENT with text 'classroom'
    idents = []
    def collect(t):
        if t.name == 'IDENT': idents.append(t.text)
        for c in t.children: collect(c)
    collect(tree)
    check("@keyword: 'classroom' produces single IDENT 'classroom'",
          idents == ['classroom'])

    # Inline @keyword
    p = load_grammar(r"""
        start g
        _ = /\s*/ @longest
        IDENT = /[a-zA-Z_]\w*/ @longest
        g = @keyword 'class' IDENT
    """)
    check("inline @keyword: 'class foo' parses", p.parse('class foo').name == 'g')
    try:
        p.parse('classa'); check("inline @keyword: 'classa' rejected", False)
    except SyntaxError:
        check("inline @keyword: 'classa' rejected", True)

    # Symbol keyword (non-word-char ending): @keyword is a harmless no-op
    p = load_grammar(r"""
        start g
        @keyword '->'
        g = '->' '-'
    """)
    check("@keyword symbol: '->-' parses",
          p.parse('->-').name == 'g')

    # ---- Test 29: @atomic directive ----
    print("\n--- Test 29: @atomic ---")
    # @atomic disables auto-ws insertion inside the marked rule.
    p = load_grammar(r"""
        start main
        _ = /\s*/ @longest
        time = [0-9]+ ':' [0-9]+ ':' [0-9]+ @atomic
        main = 'now' time 'stop'
    """)
    check("@atomic: 'now 12:34:56 stop' parses",
          p.parse('now 12:34:56 stop').name == 'main')
    check("@atomic: 'now12:34:56stop' parses",
          p.parse('now12:34:56stop').name == 'main')
    try:
        p.parse('now 12 : 34 : 56 stop')
        check("@atomic: rejects ws inside time", False)
    except SyntaxError:
        check("@atomic: rejects ws inside time", True)

    # Without @atomic, ws is inserted inside a non-atomic rule.
    p2 = load_grammar(r"""
        start main
        _ = /\s*/ @longest
        time = [0-9]+ ':' [0-9]+ ':' [0-9]+
        main = 'now' time 'stop'
    """)
    check("no @atomic: 'now 12 : 34 : 56 stop' parses",
          p2.parse('now 12 : 34 : 56 stop').name == 'main')

    # ---- Test 30: @mode — per-rule active skip set ----
    print("\n--- Test 30: @mode ---")
    # Different modes have different skip sets.  Default mode skips
    # whitespace; "dashy" mode skips dashes.
    p = load_grammar(r"""
        start s
        dash = /-/ @skip @mode dashy
        digit = /[0-9]/
        s = digit digit digit @mode dashy
    """)
    check("@mode: '123' parses in dashy", p.parse('123').name == 's')
    check("@mode: '1-2-3' parses (dashes skipped)",
          p.parse('1-2-3').name == 's')
    check("@mode: '1--2--3' parses (multiple dashes)",
          p.parse('1--2--3').name == 's')
    try:
        p.parse('1 2 3'); check("@mode dashy rejects ws", False)
    except SyntaxError:
        check("@mode dashy rejects ws", True)

    # default mode coexists with named mode
    p = load_grammar(r"""
        start prog
        ws = /\s+/ @skip                # default mode
        dash = /-/ @skip @mode dashy    # dashy mode
        digit = /[0-9]/
        prog = 'run' dashy_expr
        dashy_expr = digit digit digit @mode dashy
    """)
    check("@mode coexist: 'run 1-2-3'",
          p.parse('run 1-2-3').name == 'prog')
    check("@mode coexist: 'run   1--2--3'",
          p.parse('run   1--2--3').name == 'prog')
    try:
        p.parse('run 1 2 3')  # ws inside dashy_expr should fail
        check("@mode coexist: rejects ws inside dashy_expr", False)
    except SyntaxError:
        check("@mode coexist: rejects ws inside dashy_expr", True)

    # ---- Test 31: semantic actions {{ ... }} ----
    print("\n--- Test 31: semantic actions ---")
    # Simplest: action returns the matched text as an int
    p = load_grammar(r"""
        start n
        n = /\d+/ {{ int(_text) }}
    """)
    check("action: int(_text) for NUMBER",
          p.parse('42').value == 42)

    # Capture returns the action value (not text) of the captured rule
    p = load_grammar(r"""
        start pair
        NUMBER = /\d+/ {{ int(_text) }} @longest
        pair = a:=(NUMBER) '+' b:=(NUMBER) {{ a + b }}
    """)
    check("action+capture: 10+20 = 30",
          p.parse('10+20').value == 30)
    check("action+capture: 100+5 = 105",
          p.parse('100+5').value == 105)

    # Value propagates through a capture-only rule (no explicit action)
    p = load_grammar(r"""
        start outer
        NUMBER = /\d+/ {{ int(_text) }} @longest
        inner  = n:=(NUMBER) {{ n * 2 }}
        outer  = i:=(inner)             # no action — inherit inner's value
    """)
    check("action: value propagates via capture rule",
          p.parse('5').value == 10)

    # Multi-level expression
    p = load_grammar(r"""
        start expr
        _ = /\s*/ @longest
        NUMBER = /\d+/ {{ int(_text) }} @longest
        atom = n:=(NUMBER) {{ n }} | '(' e:=(expr) ')' {{ e }}
        expr = l:=(atom) '+' r:=(expr) {{ l + r }}
             | l:=(atom) '-' r:=(expr) {{ l - r }}
             | l:=(atom) '*' r:=(expr) {{ l * r }}
             | a:=(atom)                {{ a }}
    """)
    # Right-associative but OK for this test
    check("action: '1+2+3' = 6", p.parse('1+2+3').value == 6)
    check("action: '2*3' = 6",   p.parse('2*3').value == 6)
    check("action: '(1+2)*3' = 9",
          p.parse('(1+2)*3').value == 9)
    check("action: '10 - 5' = 5",
          p.parse('10 - 5').value == 5)

    # Action scope includes _text, _start, _end
    p = load_grammar(r"""
        start g
        g = /[a-z]+/ {{ (_text, _start, _end) }}
    """)
    t = p.parse('hello')
    check("action: _text/_start/_end exposed",
          t.value == ('hello', 0, 5))

    # Action errors are surfaced as RuntimeError
    p = load_grammar(r"""
        start g
        g = /\d+/ {{ 1 / 0 }}
    """)
    try:
        p.parse('1')
        check("action: error surfaced", False)
    except RuntimeError:
        check("action: error surfaced", True)

    # ---- Test 32a: bounded repetition variants ----
    print("\n--- Test 32a: {n}, {n,}, {n,m}, {,m} ---")
    p = load_grammar("start g\ng = 'x'{3}")
    check("{3}: 'xxx'", p.parse('xxx').text == 'xxx')
    try:  p.parse('xx');   check("{3}: 'xx' rejected", False)
    except SyntaxError:    check("{3}: 'xx' rejected", True)

    p = load_grammar("start g\ng = 'x'{2,}")
    check("{2,}: 'xx'",     p.parse('xx').text == 'xx')
    check("{2,}: 'xxxx'",   p.parse('xxxx').text == 'xxxx')
    try:  p.parse('x');     check("{2,}: 'x' rejected", False)
    except SyntaxError:     check("{2,}: 'x' rejected", True)

    p = load_grammar("start g\ng = 'x'{,3}")
    check("{,3}: ''",    p.parse('').text == '')
    check("{,3}: 'xxx'", p.parse('xxx').text == 'xxx')
    try:  p.parse('xxxx'); check("{,3}: 'xxxx' rejected", False)
    except SyntaxError:    check("{,3}: 'xxxx' rejected", True)

    p = load_grammar(r"""
        start g
        g = /x{2,4}/
    """)
    check("/x{2,4}/ on 'xxx'", p.parse('xxx').text == 'xxx')

    p = load_grammar(r"""
        start g
        g = /x{,3}/
    """)
    check("/x{,3}/ on 'xxx'", p.parse('xxx').text == 'xxx')
    check("/x{,3}/ on ''",    p.parse('').text == '')

    # ---- Test 32: ordered choice ----
    print("\n--- Test 32: ordered choice ---")
    # Same text matches two alternatives; first-listed wins.
    p = load_grammar(r"""
        start g
        g = 'abc' {{ 'first' }} | 'abc' {{ 'second' }}
    """)
    check("ordered choice: first alt wins",
          p.parse('abc').value == 'first')

    # Swap: now 'second' is listed first.
    p = load_grammar(r"""
        start g
        g = 'abc' {{ 'B' }} | 'abc' {{ 'A' }}
    """)
    check("ordered choice: reordering swaps winner",
          p.parse('abc').value == 'B')

    # If only one alt actually matches, priority is moot.
    p = load_grammar(r"""
        start g
        g = 'abcd' {{ 'long' }} | 'ab' {{ 'short' }}
    """)
    check("ordered choice: 'ab' only short matches",
          p.parse('ab').value == 'short')
    check("ordered choice: 'abcd' only long matches",
          p.parse('abcd').value == 'long')

    # Right-recursive arithmetic: ordered choice picks `*` over `+`
    # when both would match the same prefix.
    p = load_grammar(r"""
        start expr
        NUMBER = /\d+/ {{ int(_text) }} @longest
        atom = n:=(NUMBER) {{ n }} | '(' e:=(expr) ')' {{ e }}
        expr = l:=(atom) '*' r:=(expr) {{ l * r }}
             | l:=(atom) '+' r:=(expr) {{ l + r }}
             | a:=(atom)                {{ a }}
    """)
    check("ordered choice: 1+2*3 = 7 (mul-first alt)",
          p.parse('1+2*3').value == 7)
    check("ordered choice: (1+2)*3 = 9", p.parse('(1+2)*3').value == 9)

    # ---- Test 33a: @left / @right / @nonassoc precedence ----
    print("\n--- Test 33a: precedence declarations ---")
    # Left-assoc + precedence: 1+2*3 parses as 1+(2*3), 1+2+3 as (1+2)+3.
    # (String literals don't appear as ParseNode children in scannerless,
    # so the top expr has 2 children — left side, right side — and the
    # operator is inferred from the gap in the parent's text.)
    p = load_grammar(r"""
        start expr
        @left '+'
        @left '*'
        NUMBER = /[0-9]+/ @longest
        expr = expr '+' expr | expr '*' expr | NUMBER
    """)
    tree = p.parse('1+2*3')
    # `+` is outer (lower precedence): left='1', right='2*3'
    check("prec: 1+2*3 left side is `1`",
          tree.children[0].text == '1')
    check("prec: 1+2*3 right side is `2*3` (mul bound tighter)",
          tree.children[-1].text == '2*3')
    tree = p.parse('1+2+3')
    # Left-assoc: ((1+2)+3), so top's left child is `1+2` (another expr).
    check("prec: 1+2+3 left-assoc (top left is `1+2`)",
          tree.children[0].name == 'expr'
          and tree.children[0].text == '1+2')

    # Right-assoc: 2^3^4 parses as 2^(3^4).
    p = load_grammar(r"""
        start expr
        @right '^'
        NUMBER = /[0-9]+/ @longest
        expr = expr '^' expr | NUMBER
    """)
    tree = p.parse('2^3^4')
    # Top is expr; its direct child is the p0 ladder.  For right-assoc,
    # the right side nests: `3^4`.
    check("prec right: 2^3^4 parses", tree.text == '2^3^4')

    # Nonassoc: 1==2==3 rejected.
    p = load_grammar(r"""
        start expr
        @nonassoc '=='
        @left '+'
        NUMBER = /[0-9]+/ @longest
        expr = expr '==' expr | expr '+' expr | NUMBER
    """)
    check("prec nonassoc: 1+2==3+4 parses",
          p.parse('1+2==3+4').text == '1+2==3+4')
    threw = False
    try: p.parse('1==2==3')
    except SyntaxError: threw = True
    check("prec nonassoc: 1==2==3 rejected", threw)

    # Rule-reference operators in declarations (scannerless).
    p = load_grammar(r"""
        start expr
        @left plus
        @left times
        plus = '+'
        times = '*'
        NUMBER = /[0-9]+/ @longest
        expr = expr plus expr | expr times expr | NUMBER
    """)
    tree = p.parse('1+2*3')
    check("prec rule-ref ops: 1+2*3 parses", tree.text == '1+2*3')
    check("prec rule-ref ops: top right side is `2*3`",
          tree.children[-1].text == '2*3')

    # Perf sanity: ambiguous arith with precedence should be near-linear.
    # The parser without precedence would be Catalan-exponential on N ops;
    # with precedence it's a hierarchical unambiguous grammar.
    import time
    p = load_grammar(r"""
        start expr
        @left '+'
        @left '*'
        NUMBER = /[0-9]+/ @longest
        _ = /[ ]*/
        expr = expr '+' expr | expr '*' expr | NUMBER
    """)
    long_text = ' + '.join(str(i) for i in range(1, 15))  # 14 ops
    t0 = time.perf_counter()
    p.parse(long_text)
    dt = (time.perf_counter() - t0) * 1000
    # Without precedence this would take seconds; with it, well under 50ms.
    check(f"prec: 14-op chain parses in {dt:.1f}ms (<100ms)", dt < 100.0)

    # ---- Test 33b: regex `x` (verbose) and `u` (UTF-8) flags ----
    print("\n--- Test 33b: regex x / u flags ---")
    # x: whitespace and `# comment` stripped from pattern.
    p = load_grammar(r"""
        start num
        num = /-? [0-9]+ (\.[0-9]+)?/x
    """)
    check("x: matches 123", p.parse('123').text == '123')
    check("x: matches -3.14", p.parse('-3.14').text == '-3.14')

    # x: `#` starts a comment to end of pattern
    p = load_grammar(r"""
        start num
        num = / [0-9]+ # one or more digits/x
    """)
    check("x: comment ignored, matches 42", p.parse('42').text == '42')

    # x: escaped whitespace preserved as literal match
    p = load_grammar(r"""
        start s
        s = /a\ b/x
    """)
    check("x: escaped space matches literal space",
          p.parse('a b').text == 'a b')

    # u: \uHHHH emits UTF-8 byte sequence when cp > 127; `.` matches
    # one UTF-8 code point (1-4 bytes).  Byte-level — input must be
    # bytes-as-latin1 for non-ASCII to match.
    p = load_grammar(r"""
        start s
        s = /\u00e9/u
    """)
    b_e = 'é'.encode('utf-8').decode('latin-1')  # 2 bytes as str
    check("u: \\u00e9 matches UTF-8 bytes of é",
          p.parse(b_e).text == b_e)

    p = load_grammar(r"""
        start s
        s = /./u
    """)
    check("u: . matches 1-byte (ASCII)", p.parse('a').text == 'a')
    check("u: . matches 2-byte codepoint",
          p.parse('é'.encode('utf-8').decode('latin-1')).text
          == 'é'.encode('utf-8').decode('latin-1'))
    check("u: . matches 3-byte codepoint",
          p.parse('中'.encode('utf-8').decode('latin-1')).text
          == '中'.encode('utf-8').decode('latin-1'))
    check("u: . matches 4-byte codepoint",
          p.parse('𝕌'.encode('utf-8').decode('latin-1')).text
          == '𝕌'.encode('utf-8').decode('latin-1'))

    # Sanity: without u, \u00e9 gives chr(0xe9) — in Python str this
    # matches the single-codepoint 'é' directly (Python str semantics).
    p = load_grammar(r"""
        start s
        s = /\u00e9/
    """)
    check("no u: \\u00e9 matches single-codepoint 'é' in Python str",
          p.parse('é').text == 'é')

    # ---- Test 33c: Unicode character class ranges under /u ----
    print("\n--- Test 33c: Unicode class ranges under /u ---")
    # 2-byte range (all codepoints U+00A0..U+00FF)
    p = load_grammar(r"""
        start s
        s = /[\u00a0-\u00ff]+/u
    """)
    b = 'àéíóú'.encode('utf-8').decode('latin-1')
    check("u class: 2-byte range matches latin-1 supplement",
          p.parse(b).text == b)

    # Range spanning 1-byte and 2-byte tiers
    p = load_grammar(r"""
        start s
        s = /[\u0060-\u0200]/u
    """)
    check("u class: tier-spanning matches ASCII (0x60)",
          p.parse('`').text == '`')
    check("u class: tier-spanning matches 2-byte (U+00E9)",
          p.parse('\u00e9'.encode('utf-8').decode('latin-1')).text
          == '\u00e9'.encode('utf-8').decode('latin-1'))
    check("u class: tier-spanning matches boundary (U+0200)",
          p.parse('\u0200'.encode('utf-8').decode('latin-1')).text
          == '\u0200'.encode('utf-8').decode('latin-1'))
    threw = False
    try: p.parse('Z')  # 0x5A, below 0x60
    except SyntaxError: threw = True
    check("u class: rejects below-range (0x5A)", threw)
    threw = False
    try: p.parse('\u0300'.encode('utf-8').decode('latin-1'))  # above 0x200
    except SyntaxError: threw = True
    check("u class: rejects above-range (U+0300)", threw)

    # 3-byte range (e.g., CJK Unified Ideographs start)
    p = load_grammar(r"""
        start s
        s = /[\u4e00-\u9fff]+/u
    """)
    b = '\u4e2d\u6587'.encode('utf-8').decode('latin-1')  # 中文
    check("u class: 3-byte CJK range matches",
          p.parse(b).text == b)

    # Negated Unicode class should error
    threw = False
    try:
        load_grammar(r"""
            start s
            s = /[^\u00a0-\u00ff]/u
        """)
    except SyntaxError: threw = True
    check("u class: negated non-ASCII errors", threw)

    # ---- Test 33: multi-char backreference inside a predicate ----
    # Historical rough spot: evaluate_predicate's backref handling used
    # to drop the cursor after a multi-char match (only single-char
    # backrefs worked).  This exercises the deferred-cursor + zero-width
    # completion check that fixes it.
    print("\n--- Test 33: multi-char backref inside predicate ---")
    p = load_grammar(r"""
        start s
        s = cap:=([a-z]{2}) &cap [a-z]+ '!'
    """)
    # cap='ab' at pos 0-1; &cap at pos 2 checks 'ab' matches 'ab' → pass.
    check("&(cap): abab! accepted", p.parse('abab!').text == 'abab!')
    threw = False
    try: p.parse('abcd!')
    except SyntaxError: threw = True
    check("&(cap): abcd! rejected (chars don't repeat)", threw)

    p = load_grammar(r"""
        start s
        s = cap:=([a-z]{2}) !cap [a-z]+ '!'
    """)
    check("!(cap): abcd! accepted (chars differ)",
          p.parse('abcd!').text == 'abcd!')
    threw = False
    try: p.parse('abab!')
    except SyntaxError: threw = True
    check("!(cap): abab! rejected (chars repeat)", threw)

    # ---- Summary ----
    print(f"\n{'='*40}")
    print(f"Results: {passed} passed, {failed} failed")
    if failed:
        print("SOME TESTS FAILED")
    else:
        print("ALL TESTS PASSED")
