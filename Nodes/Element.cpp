/**
 * @see Element.hpp
 */

//Primary include:
#include "Element.hpp"

//Local includes:
#include "Elements.hpp"
#include "TypeUtils.hpp"
#include "../Utils/Optional.hpp"
#include "../Token.hpp"
#include "../Utils/Utils.hpp"

//System includes:
#include <algorithm>
#include <map>
#include <sstream>

namespace
{
  struct UnresolvedItem
  {
    std::shared_ptr<SubElement> pSubElement;
    std::shared_ptr<Element> pElement;

    explicit UnresolvedItem(std::shared_ptr<SubElement> pSubElem)
    : pSubElement(pSubElem),
      pElement()
    {}
    explicit UnresolvedItem(std::shared_ptr<Element> pElem)
    : pSubElement(),
      pElement(pElem)
    {}
  };

  std::map<std::string, std::shared_ptr<UnresolvedItem> > unresolvedMap;

  template<typename PARENT_T>
  void logUnresolvedDescendants(PARENT_T& rParent)
  {
    if(unresolvedMap.empty())
      return;
    Utils::Logger::appendMessages(true);
    for(auto it = unresolvedMap.begin(); it != unresolvedMap.end(); ++it)
    {
      auto pItem = it->second;
      if(pItem->pElement)
      {
        if(Types::ELEMENT_TYPE_NONE == pItem->pElement->getElementType())
          Utils::Logger::logErr(pItem->pElement->getToken(),
            "unable to resolve " + pItem->pElement->getToken()->text);
      }
      else if(pItem->pSubElement)
      {
        if(!pItem->pSubElement->isResolved())
          Utils::Logger::logErr(pItem->pSubElement->getToken(),
            "unable to resolve " + pItem->pSubElement->getToken()->text);
      }
    }
    Utils::Logger::logAppendedMessages();
  }
  
  template<typename PARENT_T>
  void resolveDescendants(PARENT_T& rParent)
  {
    std::for_each(rParent.getChildren()->children.begin(), rParent.getChildren()->children.end(),
      [&](std::shared_ptr<SubElement> pSubElement)
      {
        //Elements::validateAccessAsMember(pSubElement->getContainingElement(), pSubElement->getToken());
        if(!pSubElement->resolvesToTypeOrLiteral())
        {
          size_t count = 0;
          for(auto it = pSubElement->getChildren()->children.begin();
              it != pSubElement->getChildren()->children.end();
              ++it)
          {
            if(!it->get()->getCppTypeInfo())
            {
              auto pChild = rParent.getChildren()->findChild(it->get()->getToken()->text);
              if(pChild && pChild->resolvesToTypeOrLiteral())
              {
                it->get()->set(pChild);
                ++count;
                auto findIt = unresolvedMap.find(pChild->getToken()->text);
                if(unresolvedMap.end() != findIt)
                  unresolvedMap.erase(findIt);
              }
              else if(pChild)
              {
                if(unresolvedMap.end() == unresolvedMap.find(pChild->getToken()->text))
                  unresolvedMap[pChild->getToken()->text] = std::make_shared<UnresolvedItem>(pChild);
              }
              else if(!rParent.getChildren()->children.empty())
              {
                for(size_t v = 0; v < rParent.getChildren()->children.size(); v++)
                {
                  auto pC = rParent.getChildren()->getChild(v);
                  if(unresolvedMap.end() == unresolvedMap.find(pC->getToken()->text))
                    unresolvedMap[pC->getToken()->text] = std::make_shared<UnresolvedItem>(pC);
                }
              }
            }
          }
          pSubElement->setIsResolved(pSubElement->getChildren()->children.size() == count);          
        }
      });

    std::vector<std::string> resolvedNames;
    std::for_each(unresolvedMap.begin(), unresolvedMap.end(),
      [&](std::pair<std::string, std::shared_ptr<UnresolvedItem> > unresolvedPair)
      {
        auto pItem = unresolvedPair.second;
        if(pItem->pElement && Types::ELEMENT_TYPE_NONE != pItem->pElement->getElementType())
          resolvedNames.push_back(pItem->pElement->getToken()->text);
        if(pItem->pSubElement && !pItem->pSubElement->getChildren()->children.empty())
        {
          auto it = std::find_if(
            pItem->pSubElement->getChildren()->children.begin(),
            pItem->pSubElement->getChildren()->children.end(),
            [&](std::shared_ptr<SubElement> pSeChild)->bool
            { return !pSeChild->isResolved(); });
          //TRICKY: If no child subelements were found that are unresolved,
          //then mark this subelement as resolved.
          bool isResolved = pItem->pSubElement->getChildren()->children.end() == it;
          if(isResolved && !pItem->pSubElement->isResolved())
          {
            pItem->pSubElement->setIsResolved(isResolved);
            resolvedNames.push_back(pItem->pSubElement->getToken()->text);
          }
        }
      });

    if(resolvedNames.empty())
      return;

    std::for_each(resolvedNames.begin(), resolvedNames.end(),
      [&](std::string resolvedName){ unresolvedMap.erase(resolvedName); });
  }

} //end anonymous namespace

Types::ElementType Types::getElementType(
  const std::shared_ptr<Token> pTok,
  const std::shared_ptr<Element> pContainingElem)
{
  if(pTok->isElementActions())
    return ELEMENT_TYPE_ACTIONS;
  else if(pTok->isElementGrammar())
    return ELEMENT_TYPE_GRAMMAR;
  else if(pTok->isElementStateTable())
    return ELEMENT_TYPE_STATES;
  else if(pContainingElem && pContainingElem->getToken()->isElementInput())
  {
    //IO element types cannot be deduced until they are assigned.
    return "CONSOLE" == pTok->text ? ELEMENT_TYPE_IO_INPUT_CONSOLE :
           "FILE" == pTok->text ? ELEMENT_TYPE_IO_INPUT_FILE :
           "GUI" == pTok->text ? ELEMENT_TYPE_IO_INPUT_GUI :
           "MYSQL" == pTok->text ? ELEMENT_TYPE_IO_INPUT_MYSQL :
           "TCP_IP" == pTok->text ? ELEMENT_TYPE_IO_INPUT_TCP_IP :
           "UDP_IP" == pTok->text ? ELEMENT_TYPE_IO_INPUT_UDP_IP :
           ELEMENT_TYPE_NONE;
  }
  else if(pContainingElem && pContainingElem->getToken()->isElementOutput())
  {
    //IO element types cannot be deduced until they are assigned.
    return "CONSOLE" == pTok->text ? ELEMENT_TYPE_IO_OUTPUT_CONSOLE :
           "FILE" == pTok->text ? ELEMENT_TYPE_IO_OUTPUT_FILE :
           "GUI" == pTok->text ? ELEMENT_TYPE_IO_OUTPUT_GUI :
           "MYSQL" == pTok->text ? ELEMENT_TYPE_IO_OUTPUT_MYSQL :
           "TCP_IP" == pTok->text ? ELEMENT_TYPE_IO_OUTPUT_TCP_IP :
           "UDP_IP" == pTok->text ? ELEMENT_TYPE_IO_OUTPUT_UDP_IP :
           ELEMENT_TYPE_NONE;
  }
  return ELEMENT_TYPE_NONE;
}

Element* Node::getContainingElement()
{
  //If possible, do a table lookup.
  if(this->getToken()->pAccessedElementToken)
  {
    auto pElem = Elements::findElement(getToken()->pAccessedElementToken->text);
    if(pElem)
      return pElem.get();
  }
  //Can't do table lookup, so traverse parents until we can't go any further.
  auto pCurrent = m_pParent;
  while(m_pParent && pCurrent->getParent())
    pCurrent = pCurrent->getParent();
  if(!pCurrent && isElement())
    pCurrent = this;
  return reinterpret_cast<Element*>(pCurrent);
}

SubElement* Node::getTopMostSubElementAncestor()
{
  SubElement* pParentSubElem = getParent<SubElement>();
  while(pParentSubElem)
  {
    auto pCurSubElem = pParentSubElem->getParent<SubElement>();
    if(!pCurSubElem)
      break;
    pParentSubElem = pCurSubElem;
  }
  return pParentSubElem;
}

void Node::setIsUsedInStateTable(bool isUsedInStateTable)
{
  m_isUsedInStateTable = isUsedInStateTable;
  if(getToken()->pAccessedElementToken)
  {
    auto pAccessedTok = getToken()->pAccessedElementToken;
    if(pAccessedTok->isElementName() || pAccessedTok->isSymbolElementNameAccessed())
    {
      auto pElem = Elements::findElement(pAccessedTok->text);
      if(pElem)
        pElem->setIsUsedInStateTable(isUsedInStateTable); //Recurses until it touches every accessed element...
    }
  }
}

void Node::setCppTypeInfo(std::shared_ptr<CppTypeInfo> pCppTypeInfo)
{
  m_pCppTypeInfo = pCppTypeInfo;
  if(!m_pCppTypeInfo->pCppTypeNode)
    m_pCppTypeInfo->pCppTypeNode = this;
}

std::shared_ptr<CppTypeInfo> Node::getCppTypeInfo(bool findIfNull) const
{
  if(m_pCppTypeInfo || !findIfNull)
    return m_pCppTypeInfo;
  else
  {
    auto pNode = Elements::findSubElement(getToken());
    if(pNode)
      return pNode->getCppTypeInfo();
  }
  return std::shared_ptr<CppTypeInfo>();
}

std::shared_ptr<Node> Node::findNode(const std::string& tokenText)
{
  std::shared_ptr<Node> pNode;
  auto pChildren = getChildren();
  if(pChildren)
  {
    pNode = pChildren->findChild(tokenText);
    if(!pNode && getParent())      
      pNode = getParent()->findNode(tokenText);
  }
  return pNode;
}

bool SubElement::resolvesToTypeOrLiteralHelper(
  std::vector<std::shared_ptr<SubElement> >& rChildren)
{
  if(m_isResolved)
    return true;
  size_t count = 0;
  size_t resolvedSzBytes = 0;
  std::shared_ptr<Token> pAssignedCastType;
  for(std::shared_ptr<SubElement> pChild : rChildren)
  {
    if(pChild->isResolved())
      count++;
    else if(pChild->getChildren()->children.empty())
    {
      //No children means this token must be a type or literal.
      if(pChild->getToken()->isSymbolTyped() || pChild->getToken()->isLiteral())
        count++;
      else
      {
        auto pFoundSe = Elements::findSubElement(pChild);
        auto foundTxt = pFoundSe->getToken()->text;
        auto foundType = pFoundSe->getToken()->type;
        auto childTxt = pChild->getToken()->text;
        auto childType = pChild->getToken()->type;
        if(pFoundSe && *pFoundSe == *pChild)
          count++;
      }
    }
    else if(1 == pChild->getChildren()->children.size())
    {
      //Single child means it can only resolve to a type.
      if(pChild->getChildren()->getChild(0)->getToken()->isSymbolTyped())
        count++;
    }
    else if(pChild->getToken()->isLiteral())
      count++;
    else if(resolvesToTypeOrLiteralHelper(pChild->getChildren()->children))
      count++;
  }
  m_isResolved = getChildren()->children.size() == count;
  return m_isResolved;
}

void SubElement::set(const SubElement& se)
{
  setToken(se.getToken());
  setCppTypeInfo(se.getCppTypeInfo());
  setRelationToParent(se.getRelationToParent());
  setRelationToSibling(se.getRelationToSibling());
  m_isResolved = se.isResolved();
  getChildren()->children.clear();
  getChildren()->childTable.clear();
  for(std::shared_ptr<SubElement> pChild : se.getChildren()->children)
    getChildren()->addChild(pChild);
}

void SubElement::resolve()
{
  resolveDescendants(*this);
}

void SubElement::logUnresolved()
{
  logUnresolvedDescendants(*this);
}

bool SubElement::operator==(const SubElement& rRhs)
{ 
  if(*(this->getToken()) == *(rRhs.getToken()))
    return true;
  auto pLhsTok = this->getToken();
  auto pRhsTok = rRhs.getToken();
  if(pLhsTok->pAccessedElementToken)
  {
    auto pChildSe = Elements::findSubElement(pLhsTok);
    pLhsTok = pChildSe->getToken();
  }
  else if(pRhsTok->pAccessedElementToken)
  {
    auto pChildSe = Elements::findSubElement(pRhsTok);
    pRhsTok = pChildSe->getToken();
  }
  return *pLhsTok == *pRhsTok;
}

void Element::resolve()
{
  resolveDescendants(*this);
  if(Types::ELEMENT_TYPE_NONE == m_elementType &&
     !unresolvedMap.empty() &&
     unresolvedMap.end() == unresolvedMap.find(getToken()->text))
    unresolvedMap[getToken()->text] = std::make_shared<UnresolvedItem>(getSharedFromThis());
}

void Element::logUnresolved()
{
  logUnresolvedDescendants(*this);
}

void SubElementActionLine::setIsUsedInStateTable(bool isUsedInStateTable)
{
  for(std::shared_ptr<Token> pTok : m_rpnTokens)
  {
    if(pTok->isElementName() || pTok->isSymbolElementNameAccessed())
    {
      auto pElem = Elements::findElement(pTok->text);
      if(pElem)
        pElem->setIsUsedInStateTable(isUsedInStateTable);
    }
    auto pCurrentTok = pTok;
    if(pTok->pAccessedElementToken)
      pCurrentTok = pTok->pAccessedElementToken;
    if(pCurrentTok->isElementName() || pCurrentTok->isSymbolElementNameAccessed())
    {
      auto pElem = Elements::findElement(pCurrentTok->text);
      if(pElem)
        pElem->setIsUsedInStateTable(isUsedInStateTable);
    }
  }
}

void SubElementState::setInputMethod(std::shared_ptr<Node> pInputMethod)
{
  m_pInputMethod = pInputMethod;
  pInputMethod->setIsUsedInStateTable(true);
  if(m_pInputMethod->isSubElement() && m_pInputMethod->getToken()->pAccessedElementToken)
    m_pInputElement = Elements::findElement(m_pInputMethod->getToken()->pAccessedElementToken->text);
  else if(m_pInputMethod->isElement())
    m_pInputElement = std::dynamic_pointer_cast<Element>(m_pInputMethod);
}

