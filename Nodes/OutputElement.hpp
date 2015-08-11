/**
*  TEBNF Output Element type.
*
* @author  Jason Young
* @version 0.1
* @since   2014-04-26
*/

#ifndef OUTPUTELEMENT_HPP
#define OUTPUTELEMENT_HPP

#include "IoElement.hpp"

class OutputElement : public IoElement
{
public:
  OutputElement(const std::shared_ptr<Token>& pTok) : IoElement(pTok) {}
  virtual std::string getTypeName() const { return "OutputElement"; }
};

#endif //OUTPUTELEMENT_HPP
