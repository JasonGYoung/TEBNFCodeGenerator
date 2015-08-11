/**
 *  TEBNF Actions Element Recursive Descent Parser.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-09-09
 */

#ifndef ACTIONSELEMENTPARSER_HPP
#define ACTIONSELEMENTPARSER_HPP

//Local includes:
#include "ElementParser.hpp"
#include "../Nodes/Element.hpp"
#include "../Tokens.hpp"

//System includes:
#include <memory>
#include <string>

class ActionsElementParser : public ElementParser
{
public:
  ActionsElementParser(std::shared_ptr<Element> pCurrentElement)
  : ElementParser(pCurrentElement)
  {}
  void parse(std::shared_ptr<Tokens> pTokens);
private:
  void parseExpr(std::shared_ptr<Tokens> pTokens);
  void parseParams(std::shared_ptr<Tokens> pTokens);
};

#endif //ACTIONSELEMENTPARSER_HPP
