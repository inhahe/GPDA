// AUTO-GENERATED — do not edit.
// Generated via emit_tokenized_graph.py.
#include "tokenized.hpp"

namespace gpda_tok {

void build_json_graph(Graph& g) {
    g.nodes.reserve(39);

    g.add_node(NodeType::RuleStart);
    g.nodes[0].rule_name = "json";
    g.add_node(NodeType::RuleRef);
    g.nodes[1].value = "value";
    g.add_node(NodeType::RuleEnd);
    g.nodes[2].rule_name = "json";
    g.add_node(NodeType::RuleStart);
    g.nodes[3].rule_name = "value";
    g.add_node(NodeType::RuleRef);
    g.nodes[4].value = "object";
    g.add_node(NodeType::RuleEnd);
    g.nodes[5].rule_name = "value";
    g.add_node(NodeType::RuleRef);
    g.nodes[6].value = "array";
    g.add_node(NodeType::MatchTok);
    g.nodes[7].value = "STRING";
    g.add_node(NodeType::MatchTok);
    g.nodes[8].value = "NUMBER";
    g.add_node(NodeType::MatchTok);
    g.nodes[9].value = "TRUE_KW";
    g.add_node(NodeType::MatchTok);
    g.nodes[10].value = "FALSE_KW";
    g.add_node(NodeType::MatchTok);
    g.nodes[11].value = "NULL_KW";
    g.add_node(NodeType::RuleStart);
    g.nodes[12].rule_name = "object";
    g.add_node(NodeType::MatchTok);
    g.nodes[13].value = "LBRACE";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[15].value = "pair";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::MatchTok);
    g.nodes[17].value = "COMMA";
    g.add_node(NodeType::RuleRef);
    g.nodes[18].value = "pair";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::Split);
    g.add_node(NodeType::MatchTok);
    g.nodes[21].value = "RBRACE";
    g.add_node(NodeType::RuleEnd);
    g.nodes[22].rule_name = "object";
    g.add_node(NodeType::RuleStart);
    g.nodes[23].rule_name = "pair";
    g.add_node(NodeType::MatchTok);
    g.nodes[24].value = "STRING";
    g.add_node(NodeType::MatchTok);
    g.nodes[25].value = "COLON";
    g.add_node(NodeType::RuleRef);
    g.nodes[26].value = "value";
    g.add_node(NodeType::RuleEnd);
    g.nodes[27].rule_name = "pair";
    g.add_node(NodeType::RuleStart);
    g.nodes[28].rule_name = "array";
    g.add_node(NodeType::MatchTok);
    g.nodes[29].value = "LBRACKET";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[31].value = "value";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::MatchTok);
    g.nodes[33].value = "COMMA";
    g.add_node(NodeType::RuleRef);
    g.nodes[34].value = "value";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::Split);
    g.add_node(NodeType::MatchTok);
    g.nodes[37].value = "RBRACKET";
    g.add_node(NodeType::RuleEnd);
    g.nodes[38].rule_name = "array";

    g.nodes[0].links.push_back(1);
    g.nodes[1].links.push_back(2);
    g.nodes[3].links.push_back(4);
    g.nodes[3].links.push_back(6);
    g.nodes[3].links.push_back(7);
    g.nodes[3].links.push_back(8);
    g.nodes[3].links.push_back(9);
    g.nodes[3].links.push_back(10);
    g.nodes[3].links.push_back(11);
    g.nodes[4].links.push_back(5);
    g.nodes[6].links.push_back(5);
    g.nodes[7].links.push_back(5);
    g.nodes[8].links.push_back(5);
    g.nodes[9].links.push_back(5);
    g.nodes[10].links.push_back(5);
    g.nodes[11].links.push_back(5);
    g.nodes[12].links.push_back(13);
    g.nodes[13].links.push_back(14);
    g.nodes[14].links.push_back(15);
    g.nodes[14].links.push_back(20);
    g.nodes[15].links.push_back(16);
    g.nodes[16].links.push_back(17);
    g.nodes[16].links.push_back(19);
    g.nodes[17].links.push_back(18);
    g.nodes[18].links.push_back(16);
    g.nodes[19].links.push_back(20);
    g.nodes[20].links.push_back(21);
    g.nodes[21].links.push_back(22);
    g.nodes[23].links.push_back(24);
    g.nodes[24].links.push_back(25);
    g.nodes[25].links.push_back(26);
    g.nodes[26].links.push_back(27);
    g.nodes[28].links.push_back(29);
    g.nodes[29].links.push_back(30);
    g.nodes[30].links.push_back(31);
    g.nodes[30].links.push_back(36);
    g.nodes[31].links.push_back(32);
    g.nodes[32].links.push_back(33);
    g.nodes[32].links.push_back(35);
    g.nodes[33].links.push_back(34);
    g.nodes[34].links.push_back(32);
    g.nodes[35].links.push_back(36);
    g.nodes[36].links.push_back(37);
    g.nodes[37].links.push_back(38);

    g.rules["json"] = {0, 2};
    g.rules["value"] = {3, 5};
    g.rules["object"] = {12, 22};
    g.rules["pair"] = {23, 27};
    g.rules["array"] = {28, 38};

    g.start_rule = "json";
}

}  // namespace gpda_tok
