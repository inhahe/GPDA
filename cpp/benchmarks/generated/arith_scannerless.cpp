// AUTO-GENERATED � do not edit.
// Generated from a grammar via emit_cpp_graph.py.
#include "scannerless.hpp"

namespace gpda {

void build_arith_graph(Graph& g) {
    g.nodes.reserve(55);

    g.add_node(NodeType::RuleStart);
    g.nodes[0].name = "_";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::MatchClass);
    g.nodes[2].cclass.chars.push_back('\t');
    g.nodes[2].cclass.chars.push_back('\n');
    g.nodes[2].cclass.chars.push_back('\r');
    g.nodes[2].cclass.chars.push_back(' ');
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleEnd);
    g.nodes[4].name = "_";
    g.add_node(NodeType::RuleStart);
    g.nodes[5].name = "NUMBER";
    g.add_node(NodeType::MatchClass);
    g.nodes[6].cclass.ranges.emplace_back('0', '9');
    g.add_node(NodeType::Split);
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleEnd);
    g.nodes[9].name = "NUMBER";
    g.add_node(NodeType::RuleStart);
    g.nodes[10].name = "main";
    g.add_node(NodeType::RuleRef);
    g.nodes[11].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[12].name = "expr";
    g.add_node(NodeType::RuleRef);
    g.nodes[13].name = "_";
    g.add_node(NodeType::RuleEnd);
    g.nodes[14].name = "main";
    g.add_node(NodeType::RuleStart);
    g.nodes[15].name = "expr";
    g.add_node(NodeType::RuleRef);
    g.nodes[16].name = "term";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[18].name = "_expr_lr_tail";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleEnd);
    g.nodes[20].name = "expr";
    g.add_node(NodeType::RuleStart);
    g.nodes[21].name = "term";
    g.add_node(NodeType::RuleRef);
    g.nodes[22].name = "factor";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[24].name = "_term_lr_tail";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleEnd);
    g.nodes[26].name = "term";
    g.add_node(NodeType::RuleStart);
    g.nodes[27].name = "factor";
    g.add_node(NodeType::RuleRef);
    g.nodes[28].name = "NUMBER";
    g.add_node(NodeType::RuleEnd);
    g.nodes[29].name = "factor";
    g.add_node(NodeType::MatchChar);
    g.nodes[30].char_val = '(';
    g.add_node(NodeType::RuleRef);
    g.nodes[31].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[32].name = "expr";
    g.add_node(NodeType::RuleRef);
    g.nodes[33].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[34].char_val = ')';
    g.add_node(NodeType::RuleStart);
    g.nodes[35].name = "_term_lr_tail";
    g.add_node(NodeType::RuleRef);
    g.nodes[36].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[37].char_val = '*';
    g.add_node(NodeType::RuleRef);
    g.nodes[38].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[39].name = "factor";
    g.add_node(NodeType::RuleEnd);
    g.nodes[40].name = "_term_lr_tail";
    g.add_node(NodeType::RuleRef);
    g.nodes[41].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[42].char_val = '/';
    g.add_node(NodeType::RuleRef);
    g.nodes[43].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[44].name = "factor";
    g.add_node(NodeType::RuleStart);
    g.nodes[45].name = "_expr_lr_tail";
    g.add_node(NodeType::RuleRef);
    g.nodes[46].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[47].char_val = '+';
    g.add_node(NodeType::RuleRef);
    g.nodes[48].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[49].name = "term";
    g.add_node(NodeType::RuleEnd);
    g.nodes[50].name = "_expr_lr_tail";
    g.add_node(NodeType::RuleRef);
    g.nodes[51].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[52].char_val = '-';
    g.add_node(NodeType::RuleRef);
    g.nodes[53].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[54].name = "term";

    g.nodes[0].links.push_back(1);
    g.nodes[1].links.push_back(2);
    g.nodes[1].links.push_back(3);
    g.nodes[2].links.push_back(1);
    g.nodes[3].links.push_back(4);
    g.nodes[5].links.push_back(6);
    g.nodes[6].links.push_back(7);
    g.nodes[7].links.push_back(6);
    g.nodes[7].links.push_back(8);
    g.nodes[8].links.push_back(9);
    g.nodes[10].links.push_back(11);
    g.nodes[11].links.push_back(12);
    g.nodes[12].links.push_back(13);
    g.nodes[13].links.push_back(14);
    g.nodes[15].links.push_back(16);
    g.nodes[16].links.push_back(17);
    g.nodes[17].links.push_back(18);
    g.nodes[17].links.push_back(19);
    g.nodes[18].links.push_back(17);
    g.nodes[19].links.push_back(20);
    g.nodes[21].links.push_back(22);
    g.nodes[22].links.push_back(23);
    g.nodes[23].links.push_back(24);
    g.nodes[23].links.push_back(25);
    g.nodes[24].links.push_back(23);
    g.nodes[25].links.push_back(26);
    g.nodes[27].links.push_back(28);
    g.nodes[27].links.push_back(30);
    g.nodes[28].links.push_back(29);
    g.nodes[30].links.push_back(31);
    g.nodes[31].links.push_back(32);
    g.nodes[32].links.push_back(33);
    g.nodes[33].links.push_back(34);
    g.nodes[34].links.push_back(29);
    g.nodes[35].links.push_back(36);
    g.nodes[35].links.push_back(41);
    g.nodes[36].links.push_back(37);
    g.nodes[37].links.push_back(38);
    g.nodes[38].links.push_back(39);
    g.nodes[39].links.push_back(40);
    g.nodes[41].links.push_back(42);
    g.nodes[42].links.push_back(43);
    g.nodes[43].links.push_back(44);
    g.nodes[44].links.push_back(40);
    g.nodes[45].links.push_back(46);
    g.nodes[45].links.push_back(51);
    g.nodes[46].links.push_back(47);
    g.nodes[47].links.push_back(48);
    g.nodes[48].links.push_back(49);
    g.nodes[49].links.push_back(50);
    g.nodes[51].links.push_back(52);
    g.nodes[52].links.push_back(53);
    g.nodes[53].links.push_back(54);
    g.nodes[54].links.push_back(50);

    g.rules["_"] = {0, 4};
    g.rules["NUMBER"] = {5, 9};
    g.rules["main"] = {10, 14};
    g.rules["expr"] = {15, 20};
    g.rules["term"] = {21, 26};
    g.rules["factor"] = {27, 29};
    g.rules["_term_lr_tail"] = {35, 40};
    g.rules["_expr_lr_tail"] = {45, 50};

    g.lr_meta["term"] = "_term_lr_tail";
    g.lr_meta["expr"] = "_expr_lr_tail";

    g.stripped_names.insert("_");

    g.dfas.resize(2);
    {
        static const std::uint32_t _dfa0_trans[] = {
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
        };
        static const std::uint8_t _dfa0_accept[] = { 0, 1 };
        auto& d = g.dfas[0];
        d.num_states = 2;
        d.trans.assign(_dfa0_trans, _dfa0_trans + 512);
        d.accept.assign(_dfa0_accept, _dfa0_accept + 2);
        d.rule_name = "NUMBER";
        d.longest = true;
    }
    {
        static const std::uint32_t _dfa1_trans[] = {
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, 1, 1, ~0u, ~0u, 1, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            1, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, 1, 1, ~0u, ~0u, 1, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            1, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
            ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u,
        };
        static const std::uint8_t _dfa1_accept[] = { 1, 1 };
        auto& d = g.dfas[1];
        d.num_states = 2;
        d.trans.assign(_dfa1_trans, _dfa1_trans + 512);
        d.accept.assign(_dfa1_accept, _dfa1_accept + 2);
        d.rule_name = "_";
        d.longest = true;
    }

    g.start_rule = "main";
}

}  // namespace gpda
