/**
 *  TEBNF Grammar Element Recursive Descent Parser.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-09-05
 */

#ifndef GRAMMARPARSER_HPP
#define GRAMMARPARSER_HPP

//Local includes:
#include "ElementParser.hpp"
#include "../Nodes/Element.hpp"
#include "../Token.hpp"
#include "../Tokens.hpp"

//System includes:
#include <map>
#include <memory>
#include <vector>

class GrammarElementParser : public ElementParser
{
public:
  GrammarElementParser(std::shared_ptr<Element> pCurrentElement)
  : ElementParser(pCurrentElement)
  {}
  void parse(std::shared_ptr<Tokens> pTokens);
private:
  void addAssignedSubElementToElement(std::shared_ptr<Tokens> pTokens);
  void addCurrentTokenToAssignedSubElement(std::shared_ptr<Tokens> pTokens);
  void parseExpr(std::shared_ptr<Tokens> pTokens);
  void parseSubExpr(std::shared_ptr<Tokens> pTokens);
  void parseExprTail(std::shared_ptr<Tokens> pTokens);
  void parseNonTerm(std::shared_ptr<Tokens> pTokens);
  void parseLiteral(std::shared_ptr<Tokens> pTokens);
  void parseNonTermSymbol(std::shared_ptr<Tokens> pTokens);
  void parseSubExprSymbol(std::shared_ptr<Tokens> pTokens);
  void parseNonTermStaticVar(std::shared_ptr<Tokens> pTokens);
  void parseSubExprStaticVar(std::shared_ptr<Tokens> pTokens);
  void parseType(std::shared_ptr<Tokens> pTokens);
  void parseTypeSizeCast(std::shared_ptr<Tokens> pTokens);
  void parseRange(std::shared_ptr<Tokens> pTokens);
  void parseRangeSubExpr(std::shared_ptr<Tokens> pTokens);
  void parseRangeExprTail(std::shared_ptr<Tokens> pTokens);
};

#endif //GRAMMARPARSER_HPP
