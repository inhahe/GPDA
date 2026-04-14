// AUTO-GENERATED � do not edit.
// Generated from a grammar via emit_cpp_graph.py.
#include "scannerless.hpp"

namespace gpda {

void build_arith_ambig_graph(Graph& g) {
    g.nodes.reserve(47);

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
    g.nodes[16].name = "NUMBER";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[18].name = "_expr_lr_tail";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleEnd);
    g.nodes[20].name = "expr";
    g.add_node(NodeType::MatchChar);
    g.nodes[21].char_val = '(';
    g.add_node(NodeType::RuleRef);
    g.nodes[22].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[23].name = "expr";
    g.add_node(NodeType::RuleRef);
    g.nodes[24].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[25].char_val = ')';
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleRef);
    g.nodes[27].name = "_expr_lr_tail";
    g.add_node(NodeType::Split);
    g.add_node(NodeType::RuleStart);
    g.nodes[29].name = "_expr_lr_tail";
    g.add_node(NodeType::RuleRef);
    g.nodes[30].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[31].char_val = '+';
    g.add_node(NodeType::RuleRef);
    g.nodes[32].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[33].name = "expr";
    g.add_node(NodeType::RuleEnd);
    g.nodes[34].name = "_expr_lr_tail";
    g.add_node(NodeType::RuleRef);
    g.nodes[35].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[36].char_val = '-';
    g.add_node(NodeType::RuleRef);
    g.nodes[37].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[38].name = "expr";
    g.add_node(NodeType::RuleRef);
    g.nodes[39].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[40].char_val = '*';
    g.add_node(NodeType::RuleRef);
    g.nodes[41].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[42].name = "expr";
    g.add_node(NodeType::RuleRef);
    g.nodes[43].name = "_";
    g.add_node(NodeType::MatchChar);
    g.nodes[44].char_val = '/';
    g.add_node(NodeType::RuleRef);
    g.nodes[45].name = "_";
    g.add_node(NodeType::RuleRef);
    g.nodes[46].name = "expr";

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
    g.nodes[15].links.push_back(21);
    g.nodes[16].links.push_back(17);
    g.nodes[17].links.push_back(18);
    g.nodes[17].links.push_back(19);
    g.nodes[18].links.push_back(17);
    g.nodes[19].links.push_back(20);
    g.nodes[21].links.push_back(22);
    g.nodes[22].links.push_back(23);
    g.nodes[23].links.push_back(24);
    g.nodes[24].links.push_back(25);
    g.nodes[25].links.push_back(26);
    g.nodes[26].links.push_back(27);
    g.nodes[26].links.push_back(28);
    g.nodes[27].links.push_back(26);
    g.nodes[28].links.push_back(20);
    g.nodes[29].links.push_back(30);
    g.nodes[29].links.push_back(35);
    g.nodes[29].links.push_back(39);
    g.nodes[29].links.push_back(43);
    g.nodes[30].links.push_back(31);
    g.nodes[31].links.push_back(32);
    g.nodes[32].links.push_back(33);
    g.nodes[33].links.push_back(34);
    g.nodes[35].links.push_back(36);
    g.nodes[36].links.push_back(37);
    g.nodes[37].links.push_back(38);
    g.nodes[38].links.push_back(34);
    g.nodes[39].links.push_back(40);
    g.nodes[40].links.push_back(41);
    g.nodes[41].links.push_back(42);
    g.nodes[42].links.push_back(34);
    g.nodes[43].links.push_back(44);
    g.nodes[44].links.push_back(45);
    g.nodes[45].links.push_back(46);
    g.nodes[46].links.push_back(34);

    g.rules["_"] = {0, 4};
    g.rules["NUMBER"] = {5, 9};
    g.rules["main"] = {10, 14};
    g.rules["expr"] = {15, 20};
    g.rules["_expr_lr_tail"] = {29, 34};

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
    }

    g.start_rule = "main";
}

}  // namespace gpda
