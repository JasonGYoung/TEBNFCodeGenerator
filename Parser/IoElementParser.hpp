/**
 *  TEBNF Input/Output Element Recursive Descent Parser.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-09-09
 */

#ifndef IOELEMENTPARSER_HPP
#define IOELEMENTPARSER_HPP

#include "ElementParser.hpp"
#include "../Nodes/Element.hpp"
#include "../Tokens.hpp"

#include <memory>
#include <string>

class IoElementParser : public ElementParser
{
public:
  IoElementParser(std::shared_ptr<Element> pCurrentElement)
  : ElementParser(pCurrentElement)
  {}
  void parse(std::shared_ptr<Tokens> pTokens);
private:
  void parseExpr(std::shared_ptr<Tokens> pTokens);
  void parseExprTail(std::shared_ptr<Tokens> pTokens);
  void parseExprTerm(std::shared_ptr<Tokens> pTokens);
  void setElemType(std::shared_ptr<Token> pTok);
private:
  std::shared_ptr<SubElement> m_pAssignedSubElement;
};

#endif //IOELEMENTPARSER_HPP
