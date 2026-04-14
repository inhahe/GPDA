// AUTO-GENERATED — do not edit.
// Generated via emit_tokenized_graph.py.
#include "tokenized.hpp"

namespace gpda_tok {

void build_arith_tok_graph(Graph& g) {
    g.nodes.reserve(33);

    g.add_node(NodeType::RuleStart);
    g.nodes[0].rule_name = "main";
    g.add_node(NodeType::RuleRef);
    g.nodes[1].value = "expr";
    g.add_node(NodeType::RuleEnd);
    g.nodes[2].rule_name = "main";
    g.add_node(NodeType::RuleStart);
    g.nodes[3].rule_name = "expr";
    g.add_node(NodeType::RuleRef);
    g.nodes[4].value = "term";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[6].value = "_expr_lr_tail";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleEnd);
    g.nodes[8].rule_name = "expr";
    g.add_node(NodeType::RuleStart);
    g.nodes[9].rule_name = "term";
    g.add_node(NodeType::RuleRef);
    g.nodes[10].value = "factor";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[12].value = "_term_lr_tail";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleEnd);
    g.nodes[14].rule_name = "term";
    g.add_node(NodeType::RuleStart);
    g.nodes[15].rule_name = "factor";
    g.add_node(NodeType::MatchTok);
    g.nodes[16].value = "NUMBER";
    g.add_node(NodeType::RuleEnd);
    g.nodes[17].rule_name = "factor";
    g.add_node(NodeType::MatchTok);
    g.nodes[18].value = "LPAREN";
    g.add_node(NodeType::RuleRef);
    g.nodes[19].value = "expr";
    g.add_node(NodeType::MatchTok);
    g.nodes[20].value = "RPAREN";
    g.add_node(NodeType::RuleStart);
    g.nodes[21].rule_name = "_expr_lr_tail";
    g.add_node(NodeType::MatchTok);
    g.nodes[22].value = "PLUS";
    g.add_node(NodeType::RuleRef);
    g.nodes[23].value = "term";
    g.add_node(NodeType::RuleEnd);
    g.nodes[24].rule_name = "_expr_lr_tail";
    g.add_node(NodeType::MatchTok);
    g.nodes[25].value = "MINUS";
    g.add_node(NodeType::RuleRef);
    g.nodes[26].value = "term";
    g.add_node(NodeType::RuleStart);
    g.nodes[27].rule_name = "_term_lr_tail";
    g.add_node(NodeType::MatchTok);
    g.nodes[28].value = "STAR";
    g.add_node(NodeType::RuleRef);
    g.nodes[29].value = "factor";
    g.add_node(NodeType::RuleEnd);
    g.nodes[30].rule_name = "_term_lr_tail";
    g.add_node(NodeType::MatchTok);
    g.nodes[31].value = "SLASH";
    g.add_node(NodeType::RuleRef);
    g.nodes[32].value = "factor";

    g.nodes[0].links.push_back(1);
    g.nodes[1].links.push_back(2);
    g.nodes[3].links.push_back(4);
    g.nodes[4].links.push_back(5);
    g.nodes[5].links.push_back(6);
    g.nodes[5].links.push_back(7);
    g.nodes[6].links.push_back(5);
    g.nodes[7].links.push_back(8);
    g.nodes[9].links.push_back(10);
    g.nodes[10].links.push_back(11);
    g.nodes[11].links.push_back(12);
    g.nodes[11].links.push_back(13);
    g.nodes[12].links.push_back(11);
    g.nodes[13].links.push_back(14);
    g.nodes[15].links.push_back(16);
    g.nodes[15].links.push_back(18);
    g.nodes[16].links.push_back(17);
    g.nodes[18].links.push_back(19);
    g.nodes[19].links.push_back(20);
    g.nodes[20].links.push_back(17);
    g.nodes[21].links.push_back(22);
    g.nodes[21].links.push_back(25);
    g.nodes[22].links.push_back(23);
    g.nodes[23].links.push_back(24);
    g.nodes[25].links.push_back(26);
    g.nodes[26].links.push_back(24);
    g.nodes[27].links.push_back(28);
    g.nodes[27].links.push_back(31);
    g.nodes[28].links.push_back(29);
    g.nodes[29].links.push_back(30);
    g.nodes[31].links.push_back(32);
    g.nodes[32].links.push_back(30);

    g.rules["main"] = {0, 2};
    g.rules["expr"] = {3, 8};
    g.rules["term"] = {9, 14};
    g.rules["factor"] = {15, 17};
    g.rules["_expr_lr_tail"] = {21, 24};
    g.rules["_term_lr_tail"] = {27, 30};

    g.lr_meta["expr"] = "_expr_lr_tail";
    g.lr_meta["term"] = "_term_lr_tail";

    g.start_rule = "main";
}

}  // namespace gpda_tok
