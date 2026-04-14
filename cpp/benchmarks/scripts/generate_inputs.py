"""Generate random JSON files at various sizes for benchmarking.
Seeded so all parsers see identical input."""
import json, random, string, os, sys

def gen_value(depth):
    if depth > 4:
        return random.choice([random.randint(-1000, 1000),
                              round(random.random(), 4),
                              True, False, None])
    type_ = random.choices(
        ['int', 'float', 'str', 'bool', 'null', 'arr', 'obj'],
        weights=[3, 2, 3, 1, 1, 3, 3])[0]
    if type_ == 'int':   return random.randint(-100000, 100000)
    if type_ == 'float': return round(random.uniform(-1000, 1000), 4)
    if type_ == 'str':   return ''.join(random.choices(
        string.ascii_letters + string.digits + '_',
        k=random.randint(1, 15)))
    if type_ == 'bool':  return random.choice([True, False])
    if type_ == 'null':  return None
    if type_ == 'arr':   return [gen_value(depth+1)
                                 for _ in range(random.randint(0, 6))]
    if type_ == 'obj':
        return {
            ''.join(random.choices(string.ascii_letters, k=6)): gen_value(depth+1)
            for _ in range(random.randint(0, 5))
        }


def gen_at_least(target_bytes, seed):
    random.seed(seed)
    items = []
    while True:
        v = gen_value(0)
        items.append(v)
        text = json.dumps(items)
        if len(text) >= target_bytes:
            return text


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
        path = os.path.join(out_dir, f'{name}.json')
        with open(path, 'w', encoding='utf-8') as f:
            f.write(text)
        # Sanity: check json parses
        json.loads(text)
        print(f'{name}: {len(text):>10} bytes  ->  {path}')


if __name__ == '__main__':
    main()
