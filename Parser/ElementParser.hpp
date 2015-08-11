/**
 *  TEBNF Element Recursive Descent Parser base class.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-09-10
 */

#ifndef ELEMENTPARSER_HPP
#define ELEMENTPARSER_HPP

//Local includes:
#include "../Nodes/Element.hpp"
#include "../Nodes/Elements.hpp"
#include "../Tokens.hpp"

//System includes:
#include <memory>
#include <string>
#include <vector>

class ElementParser
{
public:
  ElementParser(std::shared_ptr<Element> pCurrentElement)
  : m_pElement(pCurrentElement),
    m_pAssignedSubElement()
  {}
  virtual void parse(std::shared_ptr<Tokens> pTokens) = 0;
  static std::vector<std::shared_ptr<Token> > parseBasicExpr(
    std::shared_ptr<Tokens> pTokens,
    Utils::optional<Token::TokenType> optEndTokType = Utils::none,
    bool returnTokensInRpn = true);
protected:
  void parseElementAs(std::shared_ptr<Tokens> pTokens);
  void parseElementEnd(std::shared_ptr<Tokens> pTokens);
  bool isElementEnded(std::shared_ptr<Tokens> pTokens);
protected:
  std::shared_ptr<Element> m_pElement;
  std::shared_ptr<SubElement> m_pAssignedSubElement;
};

#endif //ELEMENTPARSER_HPP
