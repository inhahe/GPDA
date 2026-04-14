"""Generate random arithmetic expressions at various sizes for benchmarking.
Seeded so all parsers see identical input.  A 'statement' is one expression
followed by a newline; the final file contains a very long left-associative
expression of the form `(expr op expr op expr ...)` enclosed in parens so
both left-associative (precedence grammar) and fully-ambiguous grammars
produce comparably sized parse trees.
"""
import random, os, sys

# Token-ish items that reconstruct into a single expression.
OPS = ['+', '-', '*', '/']


def gen_atom(depth):
    # Mostly numbers; occasionally parenthesised sub-expressions.
    if depth > 3 or random.random() < 0.75:
        return str(random.randint(1, 9999))
    return '(' + gen_expr(depth + 1, length=random.randint(2, 6)) + ')'


def gen_expr(depth, length):
    """Produce a string like:  atom op atom op atom ... (length atoms)"""
    parts = [gen_atom(depth)]
    for _ in range(length - 1):
        parts.append(random.choice(OPS))
        parts.append(gen_atom(depth))
    return ' '.join(parts)


def gen_at_least(target_bytes, seed):
    random.seed(seed)
    # Single massive expression is the stress case for left recursion.
    # Build it by extending until the byte target is reached.
    parts = [gen_atom(0)]
    while True:
        parts.append(random.choice(OPS))
        parts.append(gen_atom(0))
        if sum(len(p) for p in parts) + len(parts) >= target_bytes:
            return ' '.join(parts)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else 'inputs'
    os.makedirs(out_dir, exist_ok=True)
    sizes = {
        'tiny':    1_000,
        'small':   10_000,
        'medium':  100_000,
        'large':   1_000_000,
    }
    seed = 42
    for name, target in sizes.items():
        text = gen_at_least(target, seed=seed)
        path = os.path.join(out_dir, f'{name}.arith')
        with open(path, 'w', encoding='utf-8') as f:
            f.write(text)
        print(f'{name}: {len(text):>10} bytes  ->  {path}')


if __name__ == '__main__':
    main()
