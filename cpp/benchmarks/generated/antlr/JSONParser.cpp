
// Generated from grammars/JSON.g4 by ANTLR 4.13.2



#include "JSONParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct JSONParserStaticData final {
  JSONParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  JSONParserStaticData(const JSONParserStaticData&) = delete;
  JSONParserStaticData(JSONParserStaticData&&) = delete;
  JSONParserStaticData& operator=(const JSONParserStaticData&) = delete;
  JSONParserStaticData& operator=(JSONParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag jsonParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<JSONParserStaticData> jsonParserStaticData = nullptr;

void jsonParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (jsonParserStaticData != nullptr) {
    return;
  }
#else
  assert(jsonParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<JSONParserStaticData>(
    std::vector<std::string>{
      "json", "value", "obj", "pair", "arr"
    },
    std::vector<std::string>{
      "", "'true'", "'false'", "'null'", "'{'", "','", "'}'", "':'", "'['", 
      "']'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "STRING", "NUMBER", "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,12,57,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,1,0,1,0,1,0,1,1,1,1,
  	1,1,1,1,1,1,1,1,1,1,3,1,21,8,1,1,2,1,2,1,2,1,2,5,2,27,8,2,10,2,12,2,30,
  	9,2,1,2,1,2,1,2,1,2,3,2,36,8,2,1,3,1,3,1,3,1,3,1,4,1,4,1,4,1,4,5,4,46,
  	8,4,10,4,12,4,49,9,4,1,4,1,4,1,4,1,4,3,4,55,8,4,1,4,0,0,5,0,2,4,6,8,0,
  	0,61,0,10,1,0,0,0,2,20,1,0,0,0,4,35,1,0,0,0,6,37,1,0,0,0,8,54,1,0,0,0,
  	10,11,3,2,1,0,11,12,5,0,0,1,12,1,1,0,0,0,13,21,3,4,2,0,14,21,3,8,4,0,
  	15,21,5,10,0,0,16,21,5,11,0,0,17,21,5,1,0,0,18,21,5,2,0,0,19,21,5,3,0,
  	0,20,13,1,0,0,0,20,14,1,0,0,0,20,15,1,0,0,0,20,16,1,0,0,0,20,17,1,0,0,
  	0,20,18,1,0,0,0,20,19,1,0,0,0,21,3,1,0,0,0,22,23,5,4,0,0,23,28,3,6,3,
  	0,24,25,5,5,0,0,25,27,3,6,3,0,26,24,1,0,0,0,27,30,1,0,0,0,28,26,1,0,0,
  	0,28,29,1,0,0,0,29,31,1,0,0,0,30,28,1,0,0,0,31,32,5,6,0,0,32,36,1,0,0,
  	0,33,34,5,4,0,0,34,36,5,6,0,0,35,22,1,0,0,0,35,33,1,0,0,0,36,5,1,0,0,
  	0,37,38,5,10,0,0,38,39,5,7,0,0,39,40,3,2,1,0,40,7,1,0,0,0,41,42,5,8,0,
  	0,42,47,3,2,1,0,43,44,5,5,0,0,44,46,3,2,1,0,45,43,1,0,0,0,46,49,1,0,0,
  	0,47,45,1,0,0,0,47,48,1,0,0,0,48,50,1,0,0,0,49,47,1,0,0,0,50,51,5,9,0,
  	0,51,55,1,0,0,0,52,53,5,8,0,0,53,55,5,9,0,0,54,41,1,0,0,0,54,52,1,0,0,
  	0,55,9,1,0,0,0,5,20,28,35,47,54
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  jsonParserStaticData = std::move(staticData);
}

}

JSONParser::JSONParser(TokenStream *input) : JSONParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

JSONParser::JSONParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  JSONParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *jsonParserStaticData->atn, jsonParserStaticData->decisionToDFA, jsonParserStaticData->sharedContextCache, options);
}

JSONParser::~JSONParser() {
  delete _interpreter;
}

const atn::ATN& JSONParser::getATN() const {
  return *jsonParserStaticData->atn;
}

std::string JSONParser::getGrammarFileName() const {
  return "JSON.g4";
}

const std::vector<std::string>& JSONParser::getRuleNames() const {
  return jsonParserStaticData->ruleNames;
}

const dfa::Vocabulary& JSONParser::getVocabulary() const {
  return jsonParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView JSONParser::getSerializedATN() const {
  return jsonParserStaticData->serializedATN;
}


//----------------- JsonContext ------------------------------------------------------------------

JSONParser::JsonContext::JsonContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

JSONParser::ValueContext* JSONParser::JsonContext::value() {
  return getRuleContext<JSONParser::ValueContext>(0);
}

tree::TerminalNode* JSONParser::JsonContext::EOF() {
  return getToken(JSONParser::EOF, 0);
}


size_t JSONParser::JsonContext::getRuleIndex() const {
  return JSONParser::RuleJson;
}


JSONParser::JsonContext* JSONParser::json() {
  JsonContext *_localctx = _tracker.createInstance<JsonContext>(_ctx, getState());
  enterRule(_localctx, 0, JSONParser::RuleJson);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(10);
    value();
    setState(11);
    match(JSONParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ValueContext ------------------------------------------------------------------

JSONParser::ValueContext::ValueContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

JSONParser::ObjContext* JSONParser::ValueContext::obj() {
  return getRuleContext<JSONParser::ObjContext>(0);
}

JSONParser::ArrContext* JSONParser::ValueContext::arr() {
  return getRuleContext<JSONParser::ArrContext>(0);
}

tree::TerminalNode* JSONParser::ValueContext::STRING() {
  return getToken(JSONParser::STRING, 0);
}

tree::TerminalNode* JSONParser::ValueContext::NUMBER() {
  return getToken(JSONParser::NUMBER, 0);
}


size_t JSONParser::ValueContext::getRuleIndex() const {
  return JSONParser::RuleValue;
}


JSONParser::ValueContext* JSONParser::value() {
  ValueContext *_localctx = _tracker.createInstance<ValueContext>(_ctx, getState());
  enterRule(_localctx, 2, JSONParser::RuleValue);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(20);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case JSONParser::T__3: {
        enterOuterAlt(_localctx, 1);
        setState(13);
        obj();
        break;
      }

      case JSONParser::T__7: {
        enterOuterAlt(_localctx, 2);
        setState(14);
        arr();
        break;
      }

      case JSONParser::STRING: {
        enterOuterAlt(_localctx, 3);
        setState(15);
        match(JSONParser::STRING);
        break;
      }

      case JSONParser::NUMBER: {
        enterOuterAlt(_localctx, 4);
        setState(16);
        match(JSONParser::NUMBER);
        break;
      }

      case JSONParser::T__0: {
        enterOuterAlt(_localctx, 5);
        setState(17);
        match(JSONParser::T__0);
        break;
      }

      case JSONParser::T__1: {
        enterOuterAlt(_localctx, 6);
        setState(18);
        match(JSONParser::T__1);
        break;
      }

      case JSONParser::T__2: {
        enterOuterAlt(_localctx, 7);
        setState(19);
        match(JSONParser::T__2);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ObjContext ------------------------------------------------------------------

JSONParser::ObjContext::ObjContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<JSONParser::PairContext *> JSONParser::ObjContext::pair() {
  return getRuleContexts<JSONParser::PairContext>();
}

JSONParser::PairContext* JSONParser::ObjContext::pair(size_t i) {
  return getRuleContext<JSONParser::PairContext>(i);
}


size_t JSONParser::ObjContext::getRuleIndex() const {
  return JSONParser::RuleObj;
}


JSONParser::ObjContext* JSONParser::obj() {
  ObjContext *_localctx = _tracker.createInstance<ObjContext>(_ctx, getState());
  enterRule(_localctx, 4, JSONParser::RuleObj);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(35);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(22);
      match(JSONParser::T__3);
      setState(23);
      pair();
      setState(28);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == JSONParser::T__4) {
        setState(24);
        match(JSONParser::T__4);
        setState(25);
        pair();
        setState(30);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(31);
      match(JSONParser::T__5);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(33);
      match(JSONParser::T__3);
      setState(34);
      match(JSONParser::T__5);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PairContext ------------------------------------------------------------------

JSONParser::PairContext::PairContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* JSONParser::PairContext::STRING() {
  return getToken(JSONParser::STRING, 0);
}

JSONParser::ValueContext* JSONParser::PairContext::value() {
  return getRuleContext<JSONParser::ValueContext>(0);
}


size_t JSONParser::PairContext::getRuleIndex() const {
  return JSONParser::RulePair;
}


JSONParser::PairContext* JSONParser::pair() {
  PairContext *_localctx = _tracker.createInstance<PairContext>(_ctx, getState());
  enterRule(_localctx, 6, JSONParser::RulePair);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(37);
    match(JSONParser::STRING);
    setState(38);
    match(JSONParser::T__6);
    setState(39);
    value();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ArrContext ------------------------------------------------------------------

JSONParser::ArrContext::ArrContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<JSONParser::ValueContext *> JSONParser::ArrContext::value() {
  return getRuleContexts<JSONParser::ValueContext>();
}

JSONParser::ValueContext* JSONParser::ArrContext::value(size_t i) {
  return getRuleContext<JSONParser::ValueContext>(i);
}


size_t JSONParser::ArrContext::getRuleIndex() const {
  return JSONParser::RuleArr;
}


JSONParser::ArrContext* JSONParser::arr() {
  ArrContext *_localctx = _tracker.createInstance<ArrContext>(_ctx, getState());
  enterRule(_localctx, 8, JSONParser::RuleArr);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(54);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 4, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(41);
      match(JSONParser::T__7);
      setState(42);
      value();
      setState(47);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == JSONParser::T__4) {
        setState(43);
        match(JSONParser::T__4);
        setState(44);
        value();
        setState(49);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(50);
      match(JSONParser::T__8);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(52);
      match(JSONParser::T__7);
      setState(53);
      match(JSONParser::T__8);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void JSONParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  jsonParserInitialize();
#else
  ::antlr4::internal::call_once(jsonParserOnceFlag, jsonParserInitialize);
#endif
}
