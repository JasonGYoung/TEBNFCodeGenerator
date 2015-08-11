
#ifndef ELEMENTS_HPP
#define ELEMENTS_HPP

//Local includes:
#include "ActionsElement.hpp"
#include "Element.hpp"
#include "GrammarElement.hpp"
#include "InputElement.hpp"
#include "OutputElement.hpp"
#include "StateTableElement.hpp"

//System includes:
#include <map>
#include <string>
#include <vector>

class Elements
{
public:
  static std::vector<std::shared_ptr<Element> >& elements() { return get().getElementsHelper(); }
  static std::map<std::string, std::shared_ptr<Element> >& elementTable() { return get().getElementTableHelper(); }
  static std::shared_ptr<Element> findElement(const std::string& elementName)
  {
    auto findIt = elementTable().find(elementName);
    if(elementTable().end() != findIt)
      return findIt->second;
    return std::shared_ptr<Element>();
  }
  static std::vector<std::shared_ptr<SubElement> > findSubElements(const std::string& subElementName)
  {
    std::vector<std::shared_ptr<SubElement> > subElementVec;
    std::for_each(elements().begin(), elements().end(),
      [&](std::shared_ptr<Element> pElement)
      {
        auto pSubElem = pElement->getChildren()->findChild(subElementName);
        if(pSubElem)
          subElementVec.push_back(pSubElem);
      });
    return subElementVec;
  }
  static std::shared_ptr<SubElement> findSubElement(const std::string& subElementName)
  {
    auto seVec = findSubElements(subElementName);
    return seVec.empty() ? std::shared_ptr<SubElement>() : seVec[0];
  }
  static std::shared_ptr<SubElement> findSubElement(std::shared_ptr<Token> pToken)
  {
    if(pToken->pAccessedElementToken)
    {
      auto pElem = findElement(pToken->pAccessedElementToken->text);
      if(pElem)
        return pElem->getChildren()->findChild(pToken->text);
    }
    return findSubElement(pToken->text);
  }
  static std::shared_ptr<SubElement> findSubElement(std::shared_ptr<SubElement> pSubElement)
  {
    return findSubElement(pSubElement->getToken());
  }
  static bool validateAccessAsMember(Element* pContainingElement, std::shared_ptr<Token> pToken)
  {
    auto pAccessedElementTok = pToken->pAccessedElementToken;
    if(pAccessedElementTok)
    {
      //Make sure the specified element contains the subelement member.
      auto pElem = Elements::findElement(pAccessedElementTok->text);
      if(!pElem)
        Utils::Logger::logErr(pToken->lineNumber, "Undefined element \"" + pAccessedElementTok->text + "\"");
      if(!pElem->getChildren()->findChild(pToken->text))
        Utils::Logger::logErr(pToken->lineNumber, "Element \"" + pElem->getName() + "\" has no member named \"" + pToken->text + "\"");
    }
    else if(pToken->isSymbolGrammar() ||
            pToken->isSymbolActions() ||
            pToken->isStaticVariable())
    {
      //Must be a member of the containing element.
      if(!pContainingElement->findNode(pToken->text))
        Utils::Logger::logErr(pToken->lineNumber, "Element \"" + pContainingElement->getName() + "\" has no member named \"" + pToken->text + "\"");
    }
    return true;
  }
  /** @return a static instance of Elements.
   */
  static Elements& get() { static Elements elements; return elements; }
  static std::shared_ptr<Element> getNewElementInstance(const std::shared_ptr<Token>& pTok)
  {
    if(pTok->isElementActions()) return std::make_shared<ActionsElement>(pTok);
    if(pTok->isElementGrammar()) return std::make_shared<GrammarElement>(pTok);
    if(pTok->isElementInput()) return std::make_shared<InputElement>(pTok);
    if(pTok->isElementOutput()) return std::make_shared<OutputElement>(pTok);
    if(pTok->isElementStateTable()) return std::make_shared<StateTableElement>(pTok);
    return std::make_shared<Element>(pTok);
  }
private:
  /** Should never be called outside of this class. */
  std::vector<std::shared_ptr<Element> >& getElementsHelper() { return m_elements; }
  /** Should never be called outside of this class. */
  std::map<std::string, std::shared_ptr<Element> >& getElementTableHelper() { return m_elementTable; }
private:
  std::vector<std::shared_ptr<Element> > m_elements;
  std::map<std::string, std::shared_ptr<Element> > m_elementTable;
};

#endif //ELEMENTS_HPP
