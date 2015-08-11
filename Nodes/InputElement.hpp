/**
*  TEBNF Input Element type.
*
* @author  Jason Young
* @version 0.1
* @since   2014-04-26
*/

#ifndef INPUTELEMENT_HPP
#define INPUTELEMENT_HPP

#include "IoElement.hpp"

class InputElement : public IoElement
{
public:
  InputElement(const std::shared_ptr<Token>& pTok) : IoElement(pTok) {}
  virtual std::string getTypeName() const { return "InputElement"; }
};

#endif //INPUTELEMENT_HPP
