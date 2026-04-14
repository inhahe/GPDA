"""Run all parser drivers on all input sizes, collect CSV, print a
formatted comparison.  Skips our scannerless on large inputs (too slow)
unless ALL=1 is set in the environment."""
import csv, io, os, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
BIN  = os.path.join(ROOT, 'build', 'Release')
INPUTS = os.path.join(ROOT, 'benchmarks', 'inputs')

ALL = os.environ.get('ALL') == '1'

DRIVERS = [
    ('flex+bison',      'run_flex_bison.exe',      10, None),
    ('ours_tokenized',  'run_ours_tokenized.exe',   5, None),
    ('antlr4',          'run_antlr.exe',           10, None),
    # Our scannerless: medium is now feasible (~3s); large still too slow
    ('ours_scannerless','run_ours_scannerless.exe', 3, 'medium' if not ALL else None),
]

SIZES = ['tiny', 'small', 'medium', 'large']
SIZE_INDEX = {s: i for i, s in enumerate(SIZES)}


def run_driver(exe, input_label, iters):
    input_path = os.path.join(INPUTS, f'{input_label}.json')
    cmd = [os.path.join(BIN, exe), input_path, input_label, str(iters)]
    try:
        out = subprocess.check_output(cmd, text=True, timeout=600)
    except subprocess.TimeoutExpired:
        return None
    except subprocess.CalledProcessError as e:
        print(f'FAILED: {cmd}', e, file=sys.stderr)
        return None
    row = next(csv.reader(io.StringIO(out)))
    return row   # [parser, input, size, iters, min, med, max]


def main():
    results = {}  # (parser, input) -> row
    for name, exe, iters, stop_at in DRIVERS:
        for sz in SIZES:
            if stop_at and SIZE_INDEX[sz] > SIZE_INDEX[stop_at]:
                continue
            print(f'[run] {name} {sz}...', end=' ', flush=True)
            row = run_driver(exe, sz, iters)
            if row is None:
                print('SKIP')
                continue
            results[(name, sz)] = row
            print(f'median={float(row[5]):.3f} ms')

    # Pretty-print table: parsers as rows, sizes as columns, median ms.
    print()
    header = f'{"parser":<20}'
    for sz in SIZES:
        header += f'{sz:>14}'
    print(header)
    print('-' * len(header))
    parsers = ['flex+bison', 'antlr4', 'ours_tokenized', 'ours_scannerless']
    for p in parsers:
        row = f'{p:<20}'
        for sz in SIZES:
            r = results.get((p, sz))
            if r is None:
                row += f'{"(skipped)":>14}'
            else:
                med = float(r[5])
                row += f'{med:>12.3f} ms'
        print(row)

    # Also emit raw CSV
    print()
    print('--- raw (CSV) ---')
    print('parser,input,size_bytes,iters,min_ms,median_ms,max_ms')
    for key in sorted(results):
        print(','.join(results[key]))


if __name__ == '__main__':
    main()
