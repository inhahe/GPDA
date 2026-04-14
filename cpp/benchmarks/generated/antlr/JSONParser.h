
// Generated from grammars/JSON.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"




class  JSONParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, STRING = 10, NUMBER = 11, WS = 12
  };

  enum {
    RuleJson = 0, RuleValue = 1, RuleObj = 2, RulePair = 3, RuleArr = 4
  };

  explicit JSONParser(antlr4::TokenStream *input);

  JSONParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~JSONParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class JsonContext;
  class ValueContext;
  class ObjContext;
  class PairContext;
  class ArrContext; 

  class  JsonContext : public antlr4::ParserRuleContext {
  public:
    JsonContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ValueContext *value();
    antlr4::tree::TerminalNode *EOF();

   
  };

  JsonContext* json();

  class  ValueContext : public antlr4::ParserRuleContext {
  public:
    ValueContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ObjContext *obj();
    ArrContext *arr();
    antlr4::tree::TerminalNode *STRING();
    antlr4::tree::TerminalNode *NUMBER();

   
  };

  ValueContext* value();

  class  ObjContext : public antlr4::ParserRuleContext {
  public:
    ObjContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<PairContext *> pair();
    PairContext* pair(size_t i);

   
  };

  ObjContext* obj();

  class  PairContext : public antlr4::ParserRuleContext {
  public:
    PairContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *STRING();
    ValueContext *value();

   
  };

  PairContext* pair();

  class  ArrContext : public antlr4::ParserRuleContext {
  public:
    ArrContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ValueContext *> value();
    ValueContext* value(size_t i);

   
  };

  ArrContext* arr();


  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

