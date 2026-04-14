
// Generated from ../../grammars/Arith.g4 by ANTLR 4.13.2


#include "ArithListener.h"

#include "ArithParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct ArithParserStaticData final {
  ArithParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  ArithParserStaticData(const ArithParserStaticData&) = delete;
  ArithParserStaticData(ArithParserStaticData&&) = delete;
  ArithParserStaticData& operator=(const ArithParserStaticData&) = delete;
  ArithParserStaticData& operator=(ArithParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag arithParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<ArithParserStaticData> arithParserStaticData = nullptr;

void arithParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (arithParserStaticData != nullptr) {
    return;
  }
#else
  assert(arithParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<ArithParserStaticData>(
    std::vector<std::string>{
      "main", "expr"
    },
    std::vector<std::string>{
      "", "'*'", "'/'", "'+'", "'-'", "'('", "')'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "NUMBER", "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,8,27,2,0,7,0,2,1,7,1,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,3,1,14,8,
  	1,1,1,1,1,1,1,1,1,1,1,1,1,5,1,22,8,1,10,1,12,1,25,9,1,1,1,0,1,2,2,0,2,
  	0,2,1,0,1,2,1,0,3,4,27,0,4,1,0,0,0,2,13,1,0,0,0,4,5,3,2,1,0,5,6,5,0,0,
  	1,6,1,1,0,0,0,7,8,6,1,-1,0,8,14,5,7,0,0,9,10,5,5,0,0,10,11,3,2,1,0,11,
  	12,5,6,0,0,12,14,1,0,0,0,13,7,1,0,0,0,13,9,1,0,0,0,14,23,1,0,0,0,15,16,
  	10,4,0,0,16,17,7,0,0,0,17,22,3,2,1,5,18,19,10,3,0,0,19,20,7,1,0,0,20,
  	22,3,2,1,4,21,15,1,0,0,0,21,18,1,0,0,0,22,25,1,0,0,0,23,21,1,0,0,0,23,
  	24,1,0,0,0,24,3,1,0,0,0,25,23,1,0,0,0,3,13,21,23
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  arithParserStaticData = std::move(staticData);
}

}

ArithParser::ArithParser(TokenStream *input) : ArithParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

ArithParser::ArithParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  ArithParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *arithParserStaticData->atn, arithParserStaticData->decisionToDFA, arithParserStaticData->sharedContextCache, options);
}

ArithParser::~ArithParser() {
  delete _interpreter;
}

const atn::ATN& ArithParser::getATN() const {
  return *arithParserStaticData->atn;
}

std::string ArithParser::getGrammarFileName() const {
  return "Arith.g4";
}

const std::vector<std::string>& ArithParser::getRuleNames() const {
  return arithParserStaticData->ruleNames;
}

const dfa::Vocabulary& ArithParser::getVocabulary() const {
  return arithParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView ArithParser::getSerializedATN() const {
  return arithParserStaticData->serializedATN;
}


//----------------- MainContext ------------------------------------------------------------------

ArithParser::MainContext::MainContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

ArithParser::ExprContext* ArithParser::MainContext::expr() {
  return getRuleContext<ArithParser::ExprContext>(0);
}

tree::TerminalNode* ArithParser::MainContext::EOF() {
  return getToken(ArithParser::EOF, 0);
}


size_t ArithParser::MainContext::getRuleIndex() const {
  return ArithParser::RuleMain;
}

void ArithParser::MainContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ArithListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMain(this);
}

void ArithParser::MainContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ArithListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMain(this);
}

ArithParser::MainContext* ArithParser::main() {
  MainContext *_localctx = _tracker.createInstance<MainContext>(_ctx, getState());
  enterRule(_localctx, 0, ArithParser::RuleMain);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(4);
    expr(0);
    setState(5);
    match(ArithParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExprContext ------------------------------------------------------------------

ArithParser::ExprContext::ExprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* ArithParser::ExprContext::NUMBER() {
  return getToken(ArithParser::NUMBER, 0);
}

std::vector<ArithParser::ExprContext *> ArithParser::ExprContext::expr() {
  return getRuleContexts<ArithParser::ExprContext>();
}

ArithParser::ExprContext* ArithParser::ExprContext::expr(size_t i) {
  return getRuleContext<ArithParser::ExprContext>(i);
}


size_t ArithParser::ExprContext::getRuleIndex() const {
  return ArithParser::RuleExpr;
}

void ArithParser::ExprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ArithListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpr(this);
}

void ArithParser::ExprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ArithListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpr(this);
}


ArithParser::ExprContext* ArithParser::expr() {
   return expr(0);
}

ArithParser::ExprContext* ArithParser::expr(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  ArithParser::ExprContext *_localctx = _tracker.createInstance<ExprContext>(_ctx, parentState);
  ArithParser::ExprContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 2;
  enterRecursionRule(_localctx, 2, ArithParser::RuleExpr, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(13);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case ArithParser::NUMBER: {
        setState(8);
        match(ArithParser::NUMBER);
        break;
      }

      case ArithParser::T__4: {
        setState(9);
        match(ArithParser::T__4);
        setState(10);
        expr(0);
        setState(11);
        match(ArithParser::T__5);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(23);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(21);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<ExprContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExpr);
          setState(15);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(16);
          _la = _input->LA(1);
          if (!(_la == ArithParser::T__0

          || _la == ArithParser::T__1)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(17);
          expr(5);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<ExprContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExpr);
          setState(18);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(19);
          _la = _input->LA(1);
          if (!(_la == ArithParser::T__2

          || _la == ArithParser::T__3)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(20);
          expr(4);
          break;
        }

        default:
          break;
        } 
      }
      setState(25);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 2, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

bool ArithParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 1: return exprSempred(antlrcpp::downCast<ExprContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool ArithParser::exprSempred(ExprContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 4);
    case 1: return precpred(_ctx, 3);

  default:
    break;
  }
  return true;
}

void ArithParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  arithParserInitialize();
#else
  ::antlr4::internal::call_once(arithParserOnceFlag, arithParserInitialize);
#endif
}
