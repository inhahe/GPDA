"""
emit_cpp_graph.py — Build a grammar's graph in Python and emit C++ source
that reconstructs the same graph in gpda::Graph form.

Usage:
    python emit_cpp_graph.py <grammar.epeg> <output.cpp> <function_name>

The output file defines:  void <function_name>(gpda::Graph& g);
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', '..'))

from gpda_scannerless import (
    load_grammar, GraphBuilder, UnifiedBootstrap, NT, CharClass,
)


def cpp_string_literal(s):
    """Emit a C++ string literal, escaping special chars."""
    out = '"'
    for c in s:
        if c == '\\':     out += '\\\\'
        elif c == '"':    out += '\\"'
        elif c == '\n':   out += '\\n'
        elif c == '\t':   out += '\\t'
        elif c == '\r':   out += '\\r'
        elif ord(c) < 32: out += f'\\x{ord(c):02x}'
        else:             out += c
    return out + '"'


def cpp_char_literal(c):
    """Emit a C++ char literal."""
    if c == '\\':   return "'\\\\'"
    if c == '\'':   return "'\\''"
    if c == '\n':   return "'\\n'"
    if c == '\t':   return "'\\t'"
    if c == '\r':   return "'\\r'"
    if ord(c) < 32 or ord(c) > 126:
        return f"static_cast<char>({ord(c)})"
    return f"'{c}'"


def emit_graph(parser, func_name, out_file):
    """Emit C++ code that constructs the graph inside gpda::Graph& g."""
    # Collect all nodes reachable from rule starts (BFS)
    all_nodes = []
    node_id_to_index = {}
    def visit(n):
        if n.id in node_id_to_index:
            return
        node_id_to_index[n.id] = len(all_nodes)
        all_nodes.append(n)
        for link in n.links:
            visit(link)
    for name, (start, end) in parser.rules.items():
        visit(start)
        visit(end)

    # Build pred_start index map — predicate nodes reference another node
    # via the `value` attribute (in Python that's a Node reference).
    # We need to visit those too.
    def visit_pred(n):
        if n.type in (NT.PRED_NOT, NT.PRED_AND):
            visit(n.value)  # value is the start node of the sub-graph
        for link in n.links:
            pass  # already visited
    # Re-walk with predicate + subtract-check traversal
    changed = True
    while changed:
        changed = False
        snapshot = list(all_nodes)
        for n in snapshot:
            if n.type in (NT.PRED_NOT, NT.PRED_AND, NT.SUB_CHECK_NOT):
                if n.value.id not in node_id_to_index:
                    visit(n.value)
                    changed = True

    lines = []
    lines.append('// AUTO-GENERATED — do not edit.')
    lines.append('// Generated from a grammar via emit_cpp_graph.py.')
    lines.append('#include "scannerless.hpp"')
    lines.append('')
    lines.append('namespace gpda {')
    lines.append('')
    lines.append(f'void {func_name}(Graph& g) {{')

    # Reserve space
    lines.append(f'    g.nodes.reserve({len(all_nodes)});')
    lines.append('')

    # Emit nodes with their type (no links yet — we'll wire those up
    # after all nodes exist, so indices are stable).
    # Reject grammars containing semantic actions — they're Python-only
    # and silently dropping them would mislead users into thinking their
    # actions ran.
    action_nodes = [n for n in all_nodes if n.type == NT.ACTION]
    if action_nodes:
        raise ValueError(
            f"grammar contains {len(action_nodes)} semantic action "
            f"block(s); the C++ emitter does not support actions "
            f"(they are Python-only).  Remove the `{{{{ ... }}}}` "
            f"action blocks, or emit only for the Python parser.")

    type_map = {
        NT.MATCH_CHAR:   'NodeType::MatchChar',
        NT.MATCH_CLASS:  'NodeType::MatchClass',
        NT.MATCH_ANY:    'NodeType::MatchAny',
        NT.RULE_REF:     'NodeType::RuleRef',
        NT.SPLIT:        'NodeType::Split',
        NT.RULE_START:   'NodeType::RuleStart',
        NT.RULE_END:     'NodeType::RuleEnd',
        NT.PRED_NOT:     'NodeType::PredNot',
        NT.PRED_AND:     'NodeType::PredAnd',
        NT.BACKREF:      'NodeType::BackRef',
        NT.SUB_CHECK_NOT:'NodeType::SubCheckNot',
    }

    for idx, n in enumerate(all_nodes):
        tname = type_map[n.type]
        lines.append(f'    g.add_node({tname});')
        if n.type == NT.MATCH_CHAR:
            lines.append(f'    g.nodes[{idx}].char_val = {cpp_char_literal(n.value)};')
        elif n.type == NT.MATCH_CLASS:
            cc = n.value  # CharClass object
            for lo, hi in cc.ranges:
                lines.append(f'    g.nodes[{idx}].cclass.ranges.emplace_back('
                             f'{cpp_char_literal(lo)}, {cpp_char_literal(hi)});')
            for c in sorted(cc.chars):
                lines.append(f'    g.nodes[{idx}].cclass.chars.push_back({cpp_char_literal(c)});')
            if cc.negated:
                lines.append(f'    g.nodes[{idx}].cclass.negated = true;')
        elif n.type == NT.RULE_REF:
            lines.append(f'    g.nodes[{idx}].name = {cpp_string_literal(n.value)};')
        elif n.type == NT.BACKREF:
            lines.append(f'    g.nodes[{idx}].name = {cpp_string_literal(n.value)};')
        elif n.type in (NT.RULE_START, NT.RULE_END):
            if n.rule:
                lines.append(f'    g.nodes[{idx}].name = {cpp_string_literal(n.rule)};')
        elif n.type in (NT.PRED_NOT, NT.PRED_AND, NT.SUB_CHECK_NOT):
            pred_idx = node_id_to_index[n.value.id]
            lines.append(f'    g.nodes[{idx}].pred_start = {pred_idx};')
    lines.append('')

    # Emit links
    for idx, n in enumerate(all_nodes):
        for link in n.links:
            link_idx = node_id_to_index[link.id]
            lines.append(f'    g.nodes[{idx}].links.push_back({link_idx});')
    lines.append('')

    # Emit rules map
    for name, (start, end) in parser.rules.items():
        si = node_id_to_index[start.id]
        ei = node_id_to_index[end.id]
        lines.append(f'    g.rules[{cpp_string_literal(name)}] = {{{si}, {ei}}};')
    lines.append('')

    # Emit lr_meta
    if parser.lr_meta:
        for rule, tail in parser.lr_meta.items():
            lines.append(f'    g.lr_meta[{cpp_string_literal(rule)}] = '
                         f'{cpp_string_literal(tail)};')
        lines.append('')

    # Emit capture_names
    if parser.capture_names:
        for name in sorted(parser.capture_names):
            lines.append(f'    g.capture_names.insert({cpp_string_literal(name)});')
        lines.append('')

    # Emit stripped_names (skip rules whose matches are invisible).
    if parser.stripped_names:
        for name in sorted(parser.stripped_names):
            lines.append(f'    g.stripped_names.insert({cpp_string_literal(name)});')
        lines.append('')

    # Emit DFAs.  Each DFA becomes a sizeof(Dfa) entry in g.dfas; the
    # transition table is row-major (num_states * 256 uint32_t).  We
    # emit the table as a static const array to keep the generated
    # source readable and avoid hundreds of individual push_backs.
    if parser.dfas:
        total_dfas = len(parser.dfas)
        lines.append(f'    g.dfas.resize({total_dfas});')
        for dfa_idx, (rname, dfa) in enumerate(sorted(parser.dfas.items())):
            trans_table = []
            for s in range(dfa.num_states):
                for c in range(256):
                    t = dfa.trans[s][c]
                    trans_table.append('~0u' if t < 0 else str(t))
            # Emit trans as a static array and copy in
            lines.append(f'    {{')
            lines.append(f'        static const std::uint32_t '
                         f'_dfa{dfa_idx}_trans[] = {{')
            # 16 per line for readability
            for chunk_start in range(0, len(trans_table), 16):
                chunk = trans_table[chunk_start:chunk_start + 16]
                lines.append('            ' + ', '.join(chunk) + ',')
            lines.append(f'        }};')
            accept_str = ', '.join('1' if dfa.accept[s] else '0'
                                   for s in range(dfa.num_states))
            lines.append(f'        static const std::uint8_t '
                         f'_dfa{dfa_idx}_accept[] = {{ {accept_str} }};')
            lines.append(f'        auto& d = g.dfas[{dfa_idx}];')
            lines.append(f'        d.num_states = {dfa.num_states};')
            lines.append(f'        d.trans.assign(_dfa{dfa_idx}_trans, '
                         f'_dfa{dfa_idx}_trans + '
                         f'{dfa.num_states * 256});')
            lines.append(f'        d.accept.assign(_dfa{dfa_idx}_accept, '
                         f'_dfa{dfa_idx}_accept + {dfa.num_states});')
            lines.append(f'        d.rule_name = {cpp_string_literal(rname)};')
            if dfa.longest:
                lines.append(f'        d.longest = true;')
            lines.append(f'    }}')
        lines.append('')

    # Start rule
    lines.append(f'    g.start_rule = {cpp_string_literal(parser.start_rule)};')

    lines.append('}')
    lines.append('')
    lines.append('}  // namespace gpda')

    with open(out_file, 'w') as f:
        f.write('\n'.join(lines) + '\n')

    print(f'[emit_cpp_graph] Wrote {out_file}: {len(all_nodes)} nodes, '
          f'{len(parser.rules)} rules, {len(parser.dfas)} DFAs')


def main():
    args = sys.argv[1:]
    ebnf = False
    if '--ebnf' in args:
        ebnf = True
        args = [a for a in args if a != '--ebnf']
    if len(args) != 3:
        print("Usage: emit_cpp_graph.py [--ebnf] <grammar> <out.cpp> <func_name>")
        sys.exit(1)
    grammar_file, out_file, func_name = args
    with open(grammar_file) as f:
        text = f.read()
    parser = load_grammar(text, ebnf=ebnf)
    emit_graph(parser, func_name, out_file)


if __name__ == '__main__':
    main()
