"""Run arithmetic benchmark comparison:

UNAMBIGUOUS (explicit precedence levels):
    flex+bison       — LALR parser (uses %left declarations)
    antlr4           — ALL(*) predictive parser (precedence via alt ordering)
    ours_scannerless — GPDA, expr/term/factor levels
    ours_tokenized   — GPDA, expr/term/factor levels

AMBIGUOUS (single expression rule, all operators at one level):
    Skipped for flex+bison/antlr4: those tools don't let you *really* parse
    an ambiguous grammar — bison does but via precedence declarations that
    are equivalent to restructuring, and antlr4 uses alt ordering similarly.
    The honest comparison is:
        flex+bison / antlr4   — best-case tool use (practical fair fight)
    vs.
        ours on the restructured grammar AND on the pure-ambiguous grammar

    Our ambiguous case is expected to blow up: GPDA follows all paths and an
    ambiguous arithmetic expression of length n has Catalan(n) parses.

Files are grouped by size; `ops{N}.arith` contains N binary operators.
"""
import csv, io, os, subprocess, sys, time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
BIN  = os.path.join(ROOT, 'build', 'Release')
INPUTS = os.path.join(ROOT, 'benchmarks', 'inputs')


def run(exe, input_path, label, iters, timeout=60):
    cmd = [os.path.join(BIN, exe + '.exe'), input_path, label, str(iters)]
    try:
        t0 = time.time()
        out = subprocess.check_output(cmd, text=True, timeout=timeout)
        elapsed = time.time() - t0
    except subprocess.TimeoutExpired:
        return None, '(timeout)'
    except subprocess.CalledProcessError as e:
        return None, f'(error: {e.returncode})'
    row = next(csv.reader(io.StringIO(out)))
    # columns: parser,input,size,iters,min,med,max
    return row, f'med={float(row[5]):.3f}ms'


def main():
    # -- Full-size sweep, UNAMBIGUOUS grammar only --
    print('=== Unambiguous arithmetic grammar (precedence levels) ===')
    print()
    unamb = [
        ('flex_bison',       'run_flex_bison_arith',        10),
        ('antlr4',           'run_antlr_arith',             10),
        ('ours_tokenized',   'run_ours_tokenized_arith',     5),
        ('ours_scannerless', 'run_ours_scannerless_arith',   3),
    ]
    sizes = ['tiny', 'small', 'medium', 'large']
    results_unamb = {}
    for name, exe, iters in unamb:
        for sz in sizes:
            path = os.path.join(INPUTS, f'{sz}.arith')
            print(f'[run] {name:20s} {sz:8s}...', end=' ', flush=True)
            row, msg = run(exe, path, sz, iters, timeout=120)
            print(msg)
            if row is not None:
                results_unamb[(name, sz)] = row

    # -- Ambiguous: use ops-graded inputs and cap by timeout --
    print()
    print('=== Ambiguous arithmetic grammar (one-rule, all operators) ===')
    print('Note: flex+bison and antlr4 use their own unambig resolution '
          '(%left / alt order);')
    print('GPDA here uses a *truly* ambiguous grammar and explores all '
          'parse trees.')
    print()
    ambig = [
        # flex+bison/antlr4 with their ambig-via-precedence grammar
        ('flex_bison_precedence', 'run_flex_bison_arith',           10),
        ('antlr4_precedence',     'run_antlr_arith',                10),
        # GPDA on a genuinely ambiguous grammar
        ('ours_tokenized_ambig',  'run_ours_tokenized_arith_ambig',  3),
        ('ours_scannerless_ambig','run_ours_scannerless_arith_ambig',2),
    ]
    op_sizes = [5, 10, 20, 50, 100, 200, 500, 1000]
    results_amb = {}
    # For ambig targets: stop once we cross ~30s median so we don't waste
    # all afternoon on Catalan-exploding paths.
    for name, exe, iters in ambig:
        for n in op_sizes:
            path = os.path.join(INPUTS, f'ops{n}.arith')
            if not os.path.exists(path):
                continue
            print(f'[run] {name:24s} ops={n:<4d}...', end=' ', flush=True)
            row, msg = run(exe, path, f'ops{n}', iters, timeout=60)
            print(msg)
            if row is not None:
                results_amb[(name, n)] = row
                if float(row[5]) > 30_000:
                    # Too slow to try larger
                    print(f'  (capping {name}: median > 30s)')
                    break
            else:
                # timeout — bail
                break

    # -- Pretty print --
    print()
    print('=== UNAMBIGUOUS ARITHMETIC (median ms) ===')
    hdr = f'{"parser":<22}' + ''.join(f'{s:>14}' for s in sizes)
    print(hdr); print('-' * len(hdr))
    for name, _, _ in unamb:
        row = f'{name:<22}'
        for sz in sizes:
            r = results_unamb.get((name, sz))
            if r is None:
                row += f'{"(n/a)":>14}'
            else:
                row += f'{float(r[5]):>12.3f} ms'
        print(row)

    print()
    print('=== AMBIGUOUS ARITHMETIC (median ms; ops = number of '
          'binary operators) ===')
    hdr = f'{"parser":<26}' + ''.join(f'{"ops="+str(n):>12}' for n in op_sizes)
    print(hdr); print('-' * len(hdr))
    for name, _, _ in ambig:
        row = f'{name:<26}'
        for n in op_sizes:
            r = results_amb.get((name, n))
            if r is None:
                row += f'{"(n/a)":>12}'
            else:
                row += f'{float(r[5]):>9.3f} ms'
        print(row)

    print()
    print('--- CSV ---')
    print('parser,input,size_bytes,iters,min_ms,median_ms,max_ms')
    for key in sorted(results_unamb):
        print(','.join(results_unamb[key]))
    for key in sorted(results_amb, key=lambda k: (k[0], k[1])):
        print(','.join(results_amb[key]))


if __name__ == '__main__':
    main()
