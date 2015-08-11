/**
*  TEBNF State Table Element type.
*
* @author  Jason Young
* @version 0.1
* @since   2014-04-26
*/

#ifndef STATETABLEELEMENT_HPP
#define STATETABLEELEMENT_HPP

#include "Element.hpp"

#include <set>
#include <sstream>

class StateTableElement : public Element
{
public:
  explicit StateTableElement(const std::shared_ptr<Token>& pTok);
  virtual std::string getTypeName() const { return "StateTableElement"; }
  void generateCode();
private:
  bool checkRequiresWsaSession();
};

#endif //STATETABLEELEMENT_HPP
