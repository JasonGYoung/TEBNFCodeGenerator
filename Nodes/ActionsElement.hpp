/**
*  TEBNF Actions Element type.
*
* @author  Jason Young
* @version 0.1
* @since   2015-02-12
*/

#ifndef ACTIONSELEMENT_HPP
#define ACTIONSELEMENT_HPP

#include "Element.hpp"

#include <vector>

struct CppTypeInfo;

class ActionsElement : public Element
{
public:
  ActionsElement(const std::shared_ptr<Token>& pTok);
  virtual std::string getTypeName() const { return "ActionsElement"; }
  virtual std::string getMemberAccessor() const { return "::get()->"; }
  void generateCode();
  void addParam(std::shared_ptr<Token> pParamTok) { m_params.push_back(pParamTok); }
  void addArg(std::shared_ptr<Token> pParamTok) { m_args.push_back(pParamTok); }
  std::string getParamString() const;
  std::string getActionsFunctionCall() const;
private:
  std::vector<std::shared_ptr<Token> > m_params;
  std::vector<std::shared_ptr<Token> > m_args;
};

#endif // ACTIONSELEMENT_HPP
