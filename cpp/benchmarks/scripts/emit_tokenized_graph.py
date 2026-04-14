"""
emit_tokenized_graph.py — emit C++ code that builds the tokenized parser's
graph (gpda_tok::Graph) from an EPEG grammar.

Uses the Python gpda.py bootstrap to parse the grammar.
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', '..'))

from gpda import load_grammar, NT


def cpp_string_literal(s):
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


TYPE_MAP = {
    NT.MATCH_STR:   'NodeType::MatchStr',
    NT.MATCH_TOK:   'NodeType::MatchTok',
    NT.RULE_REF:    'NodeType::RuleRef',
    NT.SPLIT:        'NodeType::Split',
    NT.RULE_START:   'NodeType::RuleStart',
    NT.RULE_END:     'NodeType::RuleEnd',
    NT.PRED_NOT:     'NodeType::PredNot',
    NT.PRED_AND:     'NodeType::PredAnd',
    NT.SUB_CHECK_NOT:'NodeType::SubCheckNot',
}


def emit(gram_parser, func_name, out_file):
    # gram_parser is a GraphParser (via load_grammar(...).parser)
    all_nodes = []
    node_id_to_index = {}

    def visit(n):
        if n.id in node_id_to_index:
            return
        node_id_to_index[n.id] = len(all_nodes)
        all_nodes.append(n)
        for link in n.links:
            visit(link)

    for name, (start, end) in gram_parser.rules.items():
        visit(start); visit(end)

    # Reject grammars with semantic actions — Python-only feature.
    action_nodes = [n for n in all_nodes if n.type == NT.ACTION]
    if action_nodes:
        raise ValueError(
            f"grammar contains {len(action_nodes)} semantic action "
            f"block(s); the C++ tokenized emitter does not support "
            f"actions (they are Python-only).  Remove `{{{{ ... }}}}` "
            f"action blocks, or emit only for the Python parser.")

    # Predicate and subtract-check targets
    changed = True
    while changed:
        changed = False
        for n in list(all_nodes):
            if n.type in (NT.PRED_NOT, NT.PRED_AND, NT.SUB_CHECK_NOT):
                if n.value.id not in node_id_to_index:
                    visit(n.value); changed = True

    lines = ['// AUTO-GENERATED — do not edit.',
             '// Generated via emit_tokenized_graph.py.',
             '#include "tokenized.hpp"', '',
             'namespace gpda_tok {', '',
             f'void {func_name}(Graph& g) {{',
             f'    g.nodes.reserve({len(all_nodes)});', '']

    for idx, n in enumerate(all_nodes):
        tname = TYPE_MAP[n.type]
        lines.append(f'    g.add_node({tname});')
        if n.type == NT.MATCH_STR:
            lines.append(f'    g.nodes[{idx}].value = {cpp_string_literal(n.value)};')
        elif n.type == NT.MATCH_TOK:
            lines.append(f'    g.nodes[{idx}].value = {cpp_string_literal(n.value)};')
        elif n.type == NT.RULE_REF:
            lines.append(f'    g.nodes[{idx}].value = {cpp_string_literal(n.value)};')
        elif n.type in (NT.RULE_START, NT.RULE_END):
            if n.rule:
                lines.append(f'    g.nodes[{idx}].rule_name = {cpp_string_literal(n.rule)};')
        elif n.type in (NT.PRED_NOT, NT.PRED_AND, NT.SUB_CHECK_NOT):
            lines.append(f'    g.nodes[{idx}].pred_start = {node_id_to_index[n.value.id]};')
    lines.append('')

    for idx, n in enumerate(all_nodes):
        for link in n.links:
            lines.append(f'    g.nodes[{idx}].links.push_back({node_id_to_index[link.id]});')
    lines.append('')

    for name, (start, end) in gram_parser.rules.items():
        si = node_id_to_index[start.id]
        ei = node_id_to_index[end.id]
        lines.append(f'    g.rules[{cpp_string_literal(name)}] = {{{si}, {ei}}};')
    lines.append('')

    if gram_parser.lr_meta:
        for rule, tail in gram_parser.lr_meta.items():
            lines.append(f'    g.lr_meta[{cpp_string_literal(rule)}] = '
                         f'{cpp_string_literal(tail)};')
        lines.append('')

    if gram_parser.skip_types:
        for t in sorted(gram_parser.skip_types):
            lines.append(f'    g.skip_types.insert({cpp_string_literal(t)});')
        lines.append('')

    stripped = getattr(gram_parser, 'stripped_names', None)
    if stripped:
        for name in sorted(stripped):
            lines.append(f'    g.stripped_names.insert({cpp_string_literal(name)});')
        lines.append('')

    lines.append(f'    g.start_rule = {cpp_string_literal(gram_parser.start_rule)};')
    lines.append('}')
    lines.append('')
    lines.append('}  // namespace gpda_tok')

    with open(out_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    print(f'[emit_tokenized_graph] {out_file}: '
          f'{len(all_nodes)} nodes, {len(gram_parser.rules)} rules')


def main():
    args = sys.argv[1:]
    ebnf = False
    if '--ebnf' in args:
        ebnf = True
        args = [a for a in args if a != '--ebnf']
    if len(args) != 3:
        print("Usage: emit_tokenized_graph.py [--ebnf] <grammar> <out.cpp> <func_name>")
        sys.exit(1)
    grammar_file, out_file, func_name = args
    with open(grammar_file) as f:
        text = f.read()
    gp = load_grammar(text, ebnf=ebnf)
    # In EBNF mode, load_grammar returns GraphParser directly; in EPEG
    # mode it returns a GrammarParser wrapping the auto-lexer.
    inner = gp.parser if hasattr(gp, 'parser') else gp
    emit(inner, func_name, out_file)


if __name__ == '__main__':
    main()
