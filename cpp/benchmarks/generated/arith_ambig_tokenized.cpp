// AUTO-GENERATED — do not edit.
// Generated via emit_tokenized_graph.py.
#include "tokenized.hpp"

namespace gpda_tok {

void build_arith_ambig_tok_graph(Graph& g) {
    g.nodes.reserve(25);

    g.add_node(NodeType::RuleStart);
    g.nodes[0].rule_name = "main";
    g.add_node(NodeType::RuleRef);
    g.nodes[1].value = "expr";
    g.add_node(NodeType::RuleEnd);
    g.nodes[2].rule_name = "main";
    g.add_node(NodeType::RuleStart);
    g.nodes[3].rule_name = "expr";
    g.add_node(NodeType::MatchTok);
    g.nodes[4].value = "NUMBER";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[6].value = "_expr_lr_tail";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleEnd);
    g.nodes[8].rule_name = "expr";
    g.add_node(NodeType::MatchTok);
    g.nodes[9].value = "LPAREN";
    g.add_node(NodeType::RuleRef);
    g.nodes[10].value = "expr";
    g.add_node(NodeType::MatchTok);
    g.nodes[11].value = "RPAREN";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[13].value = "_expr_lr_tail";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleStart);
    g.nodes[15].rule_name = "_expr_lr_tail";
    g.add_node(NodeType::MatchTok);
    g.nodes[16].value = "PLUS";
    g.add_node(NodeType::RuleRef);
    g.nodes[17].value = "expr";
    g.add_node(NodeType::RuleEnd);
    g.nodes[18].rule_name = "_expr_lr_tail";
    g.add_node(NodeType::MatchTok);
    g.nodes[19].value = "MINUS";
    g.add_node(NodeType::RuleRef);
    g.nodes[20].value = "expr";
    g.add_node(NodeType::MatchTok);
    g.nodes[21].value = "STAR";
    g.add_node(NodeType::RuleRef);
    g.nodes[22].value = "expr";
    g.add_node(NodeType::MatchTok);
    g.nodes[23].value = "SLASH";
    g.add_node(NodeType::RuleRef);
    g.nodes[24].value = "expr";

    g.nodes[0].links.push_back(1);
    g.nodes[1].links.push_back(2);
    g.nodes[3].links.push_back(4);
    g.nodes[3].links.push_back(9);
    g.nodes[4].links.push_back(5);
    g.nodes[5].links.push_back(6);
    g.nodes[5].links.push_back(7);
    g.nodes[6].links.push_back(5);
    g.nodes[7].links.push_back(8);
    g.nodes[9].links.push_back(10);
    g.nodes[10].links.push_back(11);
    g.nodes[11].links.push_back(12);
    g.nodes[12].links.push_back(13);
    g.nodes[12].links.push_back(14);
    g.nodes[13].links.push_back(12);
    g.nodes[14].links.push_back(8);
    g.nodes[15].links.push_back(16);
    g.nodes[15].links.push_back(19);
    g.nodes[15].links.push_back(21);
    g.nodes[15].links.push_back(23);
    g.nodes[16].links.push_back(17);
    g.nodes[17].links.push_back(18);
    g.nodes[19].links.push_back(20);
    g.nodes[20].links.push_back(18);
    g.nodes[21].links.push_back(22);
    g.nodes[22].links.push_back(18);
    g.nodes[23].links.push_back(24);
    g.nodes[24].links.push_back(18);

    g.rules["main"] = {0, 2};
    g.rules["expr"] = {3, 8};
    g.rules["_expr_lr_tail"] = {15, 18};

    g.lr_meta["expr"] = "_expr_lr_tail";

    g.start_rule = "main";
}

}  // namespace gpda_tok
