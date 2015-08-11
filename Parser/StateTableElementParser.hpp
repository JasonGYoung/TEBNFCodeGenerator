/**
 *  TEBNF Grammar Element Recursive Descent Parser.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-09-05
 */

#ifndef STATETABLEELEMENTPARSER_HPP
#define STATETABLEELEMENTPARSER_HPP

#include "ElementParser.hpp"

class StateTableElementParser : public ElementParser
{
public:
  StateTableElementParser(std::shared_ptr<Element> pCurrentElement)
  : ElementParser(pCurrentElement)
  {}
  void parse(std::shared_ptr<Tokens> pTokens);
private:
  void parseStep(std::shared_ptr<Tokens> pTokens);
  void parseStepState(std::shared_ptr<Tokens> pTokens);
  void parseStepStateElse(std::shared_ptr<Tokens> pTokens);
  void parseStepInputOrCond(std::shared_ptr<Tokens> pTokens);
  void parseStepInputMethod(std::shared_ptr<Tokens> pTokens);
  void parseStepNextState(std::shared_ptr<Tokens> pTokens);
  void parseStepOutputOrAction(std::shared_ptr<Tokens> pTokens);
  void parseStepOutputMethod(std::shared_ptr<Tokens> pTokens);
};

#endif //STATETABLEELEMENTPARSER_HPP
