/**
 *  TEBNF Grammar Element type.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-04-26
 */

#ifndef GRAMMARELEMENT_HPP
#define GRAMMARELEMENT_HPP

#include "Element.hpp"

#include <set>

struct CppTypeInfo;

class GrammarElement : public Element
{
public:
  GrammarElement(const std::shared_ptr<Token>& pTok);
  virtual std::string getTypeName() const { return "GrammarElement"; }
  virtual std::string getMemberAccessor() const { return "::get()->"; }
  void generateCode();
  void setGrammarSize(std::shared_ptr<Token> pGrammarSizeToken) { m_pGrammarSizeToken = pGrammarSizeToken; }
  std::shared_ptr<Token> getGrammarSize() const { return m_pGrammarSizeToken; }
  void setLastTerminalSubElement(std::shared_ptr<SubElement> pLastTerminalSubElement) { m_pLastTerminalSubElement = pLastTerminalSubElement; }
  std::shared_ptr<SubElement> getLastTerminalSubElement() { return m_pLastTerminalSubElement; }
private:
  void generateMarshalUnmarshalFunction();

  void generateDeclCode(std::shared_ptr<SubElement> pThisSubElement);

  void generateCode(std::shared_ptr<SubElement> pThisSubElement);

  void generateActionsFunction();

  bool getMarshalFunction(
    std::shared_ptr<CppTypeInfo> pMarshalCppInfo,
    std::shared_ptr<SubElement> pSubElement,
    std::string& rImpl,
    std::string& rDecl,
    bool isBaseCase = true);

  bool getUnmarshalFunction(
    std::shared_ptr<CppTypeInfo> pUnmarshalCppInfo,
    std::shared_ptr<SubElement> pSubElement,
    std::string& rImpl,
    std::string& rDecl,
    bool isBaseCase = true);

private:
  std::set<std::string> m_initializerListSet;
  std::shared_ptr<Token> m_pGrammarSizeToken;
  std::shared_ptr<SubElement> m_pLastTerminalSubElement;
};

#endif //GRAMMARELEMENT_HPP
