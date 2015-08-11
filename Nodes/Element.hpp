/**
 *  TEBNF Element and SubElement types.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-04-26
 */

#ifndef ELEMENT_HPP
#define ELEMENT_HPP

//Local includes:
#include "CppTypeInfo.hpp"
#include "TypeUtils.hpp"
#include "../Scanner.hpp"
#include "../Token.hpp"
#include "../Utils/Optional.hpp"
#include "../Utils/Utils.hpp"

//System includes:
#include <memory>
#include <vector>
#include <map>

//Forward declarations:
class Node;
class Element;
class SubElement;

namespace Types
{
  enum ElementType
  {
    ELEMENT_TYPE_NONE,
    ELEMENT_TYPE_ACTIONS,
    ELEMENT_TYPE_GRAMMAR,
    ELEMENT_TYPE_STATES,
    ELEMENT_TYPE_IO_INPUT_CONSOLE,
    ELEMENT_TYPE_IO_OUTPUT_CONSOLE,
    ELEMENT_TYPE_IO_INPUT_FILE,
    ELEMENT_TYPE_IO_OUTPUT_FILE,
    ELEMENT_TYPE_IO_INPUT_GUI,
    ELEMENT_TYPE_IO_OUTPUT_GUI,
    ELEMENT_TYPE_IO_INPUT_MYSQL,
    ELEMENT_TYPE_IO_OUTPUT_MYSQL,
    ELEMENT_TYPE_IO_INPUT_TCP_IP,
    ELEMENT_TYPE_IO_OUTPUT_TCP_IP,
    ELEMENT_TYPE_IO_INPUT_UDP_IP,
    ELEMENT_TYPE_IO_OUTPUT_UDP_IP
  };

  Types::ElementType getElementType(
    const std::shared_ptr<Token> pTok,
    const std::shared_ptr<Element> pElem = std::shared_ptr<Element>());
}

template<typename CHILD_T>
struct Children
{
  std::vector<std::shared_ptr<SubElement> > children;
  std::map<std::string, std::shared_ptr<SubElement> > childTable;
  std::shared_ptr<Element> pChildElement;
  Node* pParent;

  Children()
  : children(),
    childTable(),
    pChildElement()
  {}

  //std::shared_ptr<SubElement> removeLastChild()
  //{
  //  std::shared_ptr<SubElement> pRetChild;
  //  if(!children.empty() && childTable.end() != childTable.find(children.back()->getToken()->text))
  //  {
  //    pRetChild = children.back();
  //    childTable.erase(children.back()->getToken()->text);
  //    children.pop_back();
  //  }
  //  return pRetChild;
  //}

  bool addChild(std::shared_ptr<CHILD_T> pChild,
                std::shared_ptr<Token> pValidateAccessAsMemberToken = std::shared_ptr<Token>())
  {
    if(!pChild || childTable.end() != childTable.find(pChild->getToken()->text))
      return false;
    pChild->setParent(pParent);
    if(!pChild->getToken()->isOperator() && !pChild->getToken()->isLiteral())
      childTable[pChild->getToken()->text] = pChild;
    children.push_back(pChild);

    if(pChild->getToken()->isSymbolGrammar() || pChild->getToken()->isSymbolTyped())
    {
    }

    bool isParentCppTypeSet = false;
    if(pParent->isSubElement() && !pParent->getCppTypeInfo())
    {
      pParent->setCppTypeInfo(TypeUtils::getCppTypeInfo(pChild.get(), pParent));
      isParentCppTypeSet = true;
    }

    if(pChild->getToken()->isLiteral() && !pChild->getCppTypeInfo())
      pChild->setCppTypeInfo(TypeUtils::getCppTypeInfo(pChild.get(), pChild.get()));
    if(!isParentCppTypeSet && pChild->getCppTypeInfo())
    {
      auto pLastSibling = getLastSibling(pChild);
      if((!pLastSibling || !pLastSibling->getRelationToSibling()) ||
         (pChild->getRelationToSibling() && !Token::isOperatorOr(*pChild->getRelationToSibling())))
        pParent->getCppTypeInfo()->resolvedSizeBytes += pChild->getCppTypeInfo()->typeSizeBits/CHAR_BIT;
    }

    if(pChild->isSubElement())
    {
      auto optRelToParent = pChild->getRelationToParent();
      auto optRelToSibling = pChild->getRelationToSibling();
      if((optRelToParent && Token::TYPE_NONE != *optRelToParent) ||
         (optRelToSibling && Token::TYPE_NONE != *optRelToSibling))
      {
        //The marshal/unmarshal calls for this child are already called via other
        //marshal/unmarshal functions, so no need to call at the element level.
        auto pElement = pChild->getContainingElement();
        pElement->removeGrammarUnmarshalSubElement(pChild);
      }
      auto pValidateAccessTok = pValidateAccessAsMemberToken ? pValidateAccessAsMemberToken : pChild->getToken();
      Elements::validateAccessAsMember(pChild->getContainingElement(), pValidateAccessTok); //Verify scope resolution is correct.
    }
    return true;
  }

  std::shared_ptr<CHILD_T> getChild(size_t idx)
  {
    if(idx >= children.size())
      return std::shared_ptr<CHILD_T>();
    return Utils::getItemAt(children, idx);
  }

  std::shared_ptr<SubElement> getLastSibling(std::shared_ptr<SubElement> pSubElement)
  {
    if(!pSubElement || children.size() < 2)
      return std::shared_ptr<SubElement>();
    for(size_t i = children.size() - 1; i > 0; --i)
    {
      if(children.at(i).get() == pSubElement.get())
        return children.at(i - 1); //This is safe because i is always > 0.
    }
    return std::shared_ptr<SubElement>();
  }

  std::shared_ptr<CHILD_T> getLastChild()
  {
    if(children.empty())
      return std::shared_ptr<CHILD_T>();
    return Utils::getItemAt(children, children.size() - 1);
  }

  std::shared_ptr<CHILD_T> findChild(const std::string& childName)
  {
    if(!childName.empty())
    {
      auto it = childTable.find(childName);
      if(childTable.end() != it)
        return it->second;
    }
    return std::shared_ptr<CHILD_T>();
  }

  bool isAllChildrenSame(std::shared_ptr<CppTypeInfo>& pLastCppTypeInfo)
  {
    std::shared_ptr<CppTypeInfo> pLastCppInfo;
    auto findIt = std::find_if(children.begin(), children.end(),
      [&](std::shared_ptr<SubElement> pSe)->bool
      {
        if(!pLastCppInfo)
          pLastCppInfo = pSe->getCppTypeInfo();
        else if(pLastCppInfo &&
                pSe->getCppTypeInfo() &&
                (pSe->getCppTypeInfo()->typeStr != pLastCppInfo->typeStr ||
                pSe->getCppTypeInfo()->isVector() != pLastCppInfo->isVector()))
        {
          return true;
        }
        return false;
      });
    bool isSame = children.end() == findIt;
    if(isSame)
      pLastCppTypeInfo = pLastCppInfo;
    return isSame;
  }

  bool isAllChildrenSame()
  {
    std::shared_ptr<CppTypeInfo> pLastCppInfo;
    return isAllChildrenSame(pLastCppInfo);
  }

}; //end struct Children

class Node
{
public:
  /** Constructor.
   */
  Node()
  : m_pParent(NULL),
    m_pChildren(std::make_shared<Children<SubElement> >()),
    m_pToken(),
    m_pCppTypeInfo()
  {}
  /** Constructor.
   */
  Node(const std::shared_ptr<Token>& pTok)
  : m_pParent(NULL),
    m_pChildren(std::make_shared<Children<SubElement> >()),
    m_pToken(pTok),
    m_pCppTypeInfo(),
    m_isUsedInStateTable(false)
  {}
  Element* getContainingElement();
  SubElement* getTopMostSubElementAncestor();
  virtual bool isElement() { return false; }
  virtual bool isSubElement() { return false; }
  virtual std::string getTypeName() const { return "Node"; }
  virtual std::string getMemberAccessor() const { return "."; }
  std::string getNodeAccessor() { if(auto pCppInfo = getCppTypeInfo()) return pCppInfo->getTypeNameStr() + getMemberAccessor(); return std::string(); }
  bool isUsedInStateTable() const { return m_isUsedInStateTable; }
  virtual void setIsUsedInStateTable(bool isUsedInStateTable);
  void setParent(Node* pParentNode) { m_pParent = pParentNode; }
  Node* getParent() { return m_pParent; }
  void setToken(std::shared_ptr<Token> pTok) { m_pToken = pTok; }
  std::shared_ptr<Token> getToken() const { return m_pToken; }
  void setCppTypeInfo(std::shared_ptr<CppTypeInfo> pCppTypeInfo);
  std::shared_ptr<CppTypeInfo> getCppTypeInfo(bool findIfNull = false) const;
  void setChildren(std::shared_ptr<Children<SubElement> > pChildren) { m_pChildren = pChildren; }
  std::shared_ptr<Children<SubElement> > getChildren() const { return m_pChildren; }
  bool hasChildren() { return m_pChildren && !m_pChildren->children.empty(); }
  template<typename T> T* getParent() { return NULL != m_pParent ? dynamic_cast<T*>(m_pParent) : NULL; }
  std::shared_ptr<Node> findNode(const std::string& tokenText);
  bool isType(const std::string& typeName) { return getTypeName() == typeName; }
private:
  Node* m_pParent;
  std::shared_ptr<Children<SubElement> > m_pChildren;
  std::shared_ptr<Token> m_pToken;
  std::shared_ptr<CppTypeInfo> m_pCppTypeInfo;
  bool m_isUsedInStateTable;
};

class SubElement : public Node, public std::enable_shared_from_this<SubElement>
{
public:
  /** Constructor.
   */
  SubElement(const std::shared_ptr<SubElement>& pSubElement)
  : Node(),
    m_isResolved(false),
    m_optRelationToParent(),
    m_optRelationToSibling(),
    m_pAssignCastToType()
  { getChildren()->pParent = this; set(*pSubElement);  /*TRICKY: Must set the parent first.*/ }
  /** Constructor.
   */
  SubElement(const std::shared_ptr<Token>& pTok)
  : Node(pTok),
    m_isResolved(false),
    m_optRelationToParent(),
    m_optRelationToSibling(),
    m_pAssignCastToType()
  { getChildren()->pParent = this; }
  std::shared_ptr<SubElement> getSharedFromThis() { return shared_from_this(); }
  void resolve();
  void logUnresolved();
  bool resolvesToTypeOrLiteral() { return resolvesToTypeOrLiteralHelper(getChildren()->children); }
  bool isResolved() const { return m_isResolved; }
  void setIsResolved(bool isResolvedVal) { m_isResolved = isResolvedVal; }
  void set(const SubElement& se);
  virtual std::string getTypeName() const { return "SubElement"; }
  virtual bool isSubElement() { return true; }
  void setRelationToParent(Utils::optional<Token::TokenType> optTokType) { m_optRelationToParent = optTokType; }
  Utils::optional<Token::TokenType> getRelationToParent() const { return m_optRelationToParent; }
  void setRelationToSibling(Utils::optional<Token::TokenType> optTokType) { m_optRelationToSibling = optTokType; }
  Utils::optional<Token::TokenType> getRelationToSibling() const { return m_optRelationToSibling; }
  void setAssignCastToType(std::shared_ptr<Token> pTok) { m_pAssignCastToType = pTok; }
  std::shared_ptr<Token> getAssignCastToType() const { return m_pAssignCastToType; }
  bool operator==(const SubElement& rRhs);
protected:
  bool m_isResolved;
  Utils::optional<Token::TokenType> m_optRelationToParent;
  Utils::optional<Token::TokenType> m_optRelationToSibling;
  std::shared_ptr<Token> m_pAssignCastToType;
private:
  bool resolvesToTypeOrLiteralHelper(std::vector<std::shared_ptr<SubElement> >& rChildren);
};

class Element : public Node, public std::enable_shared_from_this<Element>
{
public:
  Element(const std::shared_ptr<Token>& pTok)
  : Node(pTok),
    m_name(),
    m_unmarshallingSubElements(),
    m_elementType(Types::getElementType(pTok)),
    m_isAsElement(false),
    m_isElementEnded(false)
  {
    getChildren()->pParent = this;
    Utils::resetCppVarNameAccessedElementNames();
  }
  std::shared_ptr<Element> getSharedFromThis() { return shared_from_this(); }  
  void resolve();
  void logUnresolved();
  virtual void generateCode() {}
  virtual std::string getTypeName() const { return "Element"; }
  virtual bool isElement() { return true; }
  void setName(const std::string& name) { m_name = name; }
  std::string getName() const { return m_name; }
  void addGrammarUnmarshalSubElement(std::shared_ptr<SubElement> pSe) { m_unmarshallingSubElements.push_back(pSe); }
  void removeGrammarUnmarshalSubElement(std::shared_ptr<SubElement> pSe)
  {
    auto it = Utils::findNodeIf(m_unmarshallingSubElements, pSe);
    if(it != m_unmarshallingSubElements.end())
      m_unmarshallingSubElements.erase(it);
  }
  std::vector<std::shared_ptr<SubElement> >& getGrammarUnmarshalSubElements() { return m_unmarshallingSubElements; }
  void setElementType(Types::ElementType elemType) { m_elementType = elemType; }
  Types::ElementType getElementType() const { return m_elementType; }
  void setAsElement(bool isAsElem) { m_isAsElement = isAsElem; }
  bool isAsElement() const { return m_isAsElement; }
  void setIsElementEnded(bool isEnded) { m_isElementEnded = isEnded; }
  bool isElementEnded() const { return m_isElementEnded; }
  bool operator==(const Element& rRhs) { return *(this->getToken()) == *(rRhs.getToken()); }
private:
  std::string m_name;
  std::vector<std::shared_ptr<SubElement> > m_unmarshallingSubElements;
  Types::ElementType m_elementType;
  bool m_isAsElement;
  bool m_isElementEnded;
};

class SubElementActionLine : public SubElement
{
public:
  /** Constructor.
   * @param[in] rpnTokens - tokens comprising the action line arranged in
   *   reverse-polish notation (RPN).
   */
  explicit SubElementActionLine(std::vector<std::shared_ptr<Token> >& rpnTokens)
  : SubElement(rpnTokens[0]),
    m_rpnTokens(rpnTokens)
  {}
  virtual std::string getTypeName() const { return "SubElementActionLine"; }
  virtual void setIsUsedInStateTable(bool isUsedInStateTable);
  std::vector<std::shared_ptr<Token> > getRpnTokens() const { return m_rpnTokens; }
private:
  /** Tokens comprising the action line arranged in reverse-polish notation (RPN). */
  std::vector<std::shared_ptr<Token> > m_rpnTokens;
};

class SubElementState : public SubElement
{
public:
  //State | Input or Condition | Input Method | Next State | Output or Action | Output Method
  enum StepType
  {
    STEP_TYPE_NONE,
    STATE,
    STATE_ELSE,
    INPUT_OR_CONDITION,
    INPUT_METHOD,
    NEXT_STATE,
    OUTPUT_OR_ACTION,
    OUTPUT_METHOD
  };
  SubElementState(const std::shared_ptr<Token>& pStateTok)
  : SubElement(pStateTok),
    m_stepType(),
    m_state(pStateTok->text),
    m_pInputOrCondition(),
    m_pInputMethod(),
    m_pInputElement(),
    m_nextState(),
    m_pOutputOrAction(),
    m_pOutputElement()
  {}
  virtual std::string getTypeName() const { return "SubElementState"; }
  StepType getStepType() const { return m_stepType; }
  std::string getState() const { return m_state; }
  std::shared_ptr<Node> getInputOrCondition() const { return m_pInputOrCondition; }
  std::shared_ptr<Node> getInputMethod() const { return m_pInputMethod; }
  std::shared_ptr<Element> getInputElement() const { return m_pInputElement; }
  std::string getNextState() const { return m_nextState; }
  std::shared_ptr<Node> getOutputOrAction() const { return m_pOutputOrAction; }
  std::shared_ptr<Element> getOutputElement() const { return m_pOutputElement; }
  void setStepType(StepType stepType) { m_stepType = stepType; }
  void setInputOrCondition(std::shared_ptr<Node> pNode) { m_pInputOrCondition = pNode; pNode->setIsUsedInStateTable(true); }
  void setInputMethod(std::shared_ptr<Node> pInputMethod);
  void setNextState(const std::string& nextState) { m_nextState = nextState; }
  void setOutputOrAction(std::shared_ptr<Node> pNode) { m_pOutputOrAction = pNode; m_pOutputOrAction->setIsUsedInStateTable(true); }
  void setOutputMethod(std::shared_ptr<Element> pOutputElement) { m_pOutputElement = pOutputElement; m_pOutputElement->setIsUsedInStateTable(true); }
private:
  StepType m_stepType;
  std::string m_state;
  std::shared_ptr<Node> m_pInputOrCondition;
  std::shared_ptr<Node> m_pInputMethod;
  std::shared_ptr<Element> m_pInputElement;
  std::string m_nextState;
  std::shared_ptr<Node> m_pOutputOrAction;
  std::shared_ptr<Element> m_pOutputElement;
};

#endif //ELEMENT_HPP
