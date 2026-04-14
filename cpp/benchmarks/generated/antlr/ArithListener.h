
// Generated from ../../grammars/Arith.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "ArithParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by ArithParser.
 */
class  ArithListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterMain(ArithParser::MainContext *ctx) = 0;
  virtual void exitMain(ArithParser::MainContext *ctx) = 0;

  virtual void enterExpr(ArithParser::ExprContext *ctx) = 0;
  virtual void exitExpr(ArithParser::ExprContext *ctx) = 0;


};

