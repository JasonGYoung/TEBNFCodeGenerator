/**
 *  TEBNF Grammar Element Recursive Descent Parser.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-09-05
 */

//Primary include:
#include "GrammarElementParser.hpp"

//Local includes:
#include "../Nodes/CppTypeInfo.hpp"
#include "../Utils/Utils.hpp"

//System includes:
#include <regex>

namespace
{
  std::shared_ptr<SubElement> pTypeSizeCastSubElement;
  Token::TokenType assignTokTypeBeforeTypeSizeCast;
}

void GrammarElementParser::addAssignedSubElementToElement(std::shared_ptr<Tokens> pTokens)
{
  if(pTokens->lastToken()->isSymbolGrammarSize())
    return;
  m_pAssignedSubElement = std::make_shared<SubElement>(pTokens->lastToken());  
  auto assignedName = Utils::getCppVarName(m_pAssignedSubElement->getToken()->text);
  auto parentName = Utils::getCppVarName(m_pElement->getName());
  if(parentName == assignedName)
    Utils::Logger::logErr(m_pAssignedSubElement->getToken(), "Subelement cannot have same name as containing element");
  m_pElement->getChildren()->addChild(m_pAssignedSubElement);
}

namespace
{
  bool parseTypedSymbol(std::shared_ptr<Tokens> pTokens,
                        std::shared_ptr<SubElement>& pSubElement)
  {
    size_t currentTokenIndex = pSubElement->getToken()->index;
    auto pLastToken = pTokens->peekToken(1, Tokens::BACKWARD);
    auto pLastLastToken = pTokens->peekToken(2, Tokens::BACKWARD);
    if(!pSubElement->getToken()->isSymbolTyped())
      return false;
    std::smatch res;
    std::regex reg("(INT_STR|FLOAT_STR|BIT|BYTE|FLOAT|INT).*");
    if(!std::regex_match(pSubElement->getToken()->text.cbegin(),
       pSubElement->getToken()->text.cend(), res, reg))
      return false;
    pSubElement->setCppTypeInfo(
      TypeUtils::getCppTypeInfo(pSubElement.get(),
      std::make_shared<SubElement>(pLastLastToken).get()));
    return true;
  }
}

void GrammarElementParser::addCurrentTokenToAssignedSubElement(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  
  auto pSubElement = Elements::findSubElement(pTok);
  if(!pSubElement)
    pSubElement = std::make_shared<SubElement>(pTok);
  else
  {
    pSubElement = pSubElement->getSharedFromThis();
    pSubElement->setToken(pTok);
  }
  if(pSubElement->getToken()->isSymbolGrammarSize())
    std::dynamic_pointer_cast<GrammarElement>(m_pElement)->setGrammarSize(pTok);
  else if(m_pAssignedSubElement)
  {
    if(pSubElement->getToken()->isSymbolTyped())
      parseTypedSymbol(pTokens, pSubElement);
    if(pSubElement->getToken()->isSymbolGrammar() ||
       pSubElement->getToken()->isSymbolTyped() ||
       pSubElement->getToken()->isLiteral() ||
       pSubElement->getToken()->isStaticVariable())
    {
      auto pLastToken = pTokens->peekToken(1, Tokens::BACKWARD);
      auto lastTokType = pTypeSizeCastSubElement ? assignTokTypeBeforeTypeSizeCast : pLastToken->type;
      if(!m_pAssignedSubElement->getAssignCastToType() &&
        (pLastToken->isOperatorAssignment() || pTypeSizeCastSubElement))
      {
        pSubElement->setRelationToParent(lastTokType);
        m_pAssignedSubElement->setAssignCastToType(pSubElement->getToken());
        m_pElement->addGrammarUnmarshalSubElement(m_pAssignedSubElement);
      }
      else if(!m_pAssignedSubElement->getChildren()->children.empty())
      {
        pSubElement->setRelationToSibling(pLastToken->type);
      }
      //TRICKY: Must check using pTok because pAccessedElementToken is not set in
      //the original token assigned to the subelement.
      m_pAssignedSubElement->getChildren()->addChild(pSubElement, pTok);
      if(!pSubElement->getToken()->isSymbolTyped() &&
        !m_pAssignedSubElement->getCppTypeInfo()->isStaticVariable() &&
        !m_pAssignedSubElement->getCppTypeInfo()->isVector())
      {
        if(!m_pAssignedSubElement->getCppTypeInfo()->pTypeRange)
          m_pAssignedSubElement->getCppTypeInfo()->pTypeRange = std::make_shared<TypeRange>();
        m_pAssignedSubElement->getCppTypeInfo()->pTypeRange->pMinToken = std::make_shared<Token>();
        m_pAssignedSubElement->getCppTypeInfo()->pTypeRange->vectorType = TYPE_VEC_UNBOUNDED;
      }

      std::shared_ptr<CppTypeInfo> pLastCppTypeInfo;
      if(pTokens->peekToken(1, Tokens::FORWARD)->isOperatorTermination() &&
        m_pAssignedSubElement->getChildren()->isAllChildrenSame(pLastCppTypeInfo) &&
        pLastCppTypeInfo->isLiteral())
      {
        //All literals, so set the range of the vector.
        size_t sz = m_pAssignedSubElement->getCppTypeInfo()->resolvedSizeBytes;
        m_pAssignedSubElement->getCppTypeInfo()->pTypeRange = std::make_shared<TypeRange>(0, sz);
      }

      if(pTypeSizeCastSubElement)
      {
        if(Token::isOperatorAssignment(lastTokType))
          m_pAssignedSubElement->getCppTypeInfo()->pTypeSizeCastSubElement = pTypeSizeCastSubElement;
        pTypeSizeCastSubElement.reset();
      }

      auto pGrammarElement = std::dynamic_pointer_cast<GrammarElement>(m_pElement);
      if(pSubElement->getCppTypeInfo() &&
         pSubElement->getCppTypeInfo()->pTypeRange &&
         pSubElement->getCppTypeInfo()->pTypeRange->isVectorUnbounded())
      {
        pGrammarElement->setLastTerminalSubElement(m_pAssignedSubElement);
      }
      else if(pSubElement->getToken()->isSymbolTyped() &&
              pGrammarElement->getLastTerminalSubElement())
      {
        pGrammarElement->setLastTerminalSubElement(std::shared_ptr<SubElement>());
      }
    }
  }
  else
    Utils::Logger::logErr(pTok, "Unable to assign to subelement");
}

void GrammarElementParser::parse(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(pTok->isElementGrammar())
  {
    pTok = pTokens->nextToken();
    if(!pTok->isElementName())
      Utils::Logger::logErr(pTok, "Expected element name");
    pTokens->nextToken();
  }
  parseExpr(pTokens);
}

void GrammarElementParser::parseExpr(std::shared_ptr<Tokens> pTokens)
{
  if(m_pElement->isElementEnded()) return;
  parseNonTerm(pTokens);
  parseExprTail(pTokens);
}

void GrammarElementParser::parseSubExpr(std::shared_ptr<Tokens> pTokens)
{
  if(m_pElement->isElementEnded()) return;
  auto pTok = pTokens->peekToken();
  if(pTok->isLiteral())
    parseLiteral(pTokens);
  else if(pTok->isSymbolGrammar() || pTok->isSymbolGrammarSize())
    parseSubExprSymbol(pTokens);
  else if(pTok->isStaticVariable())
  {
    parseSubExprStaticVar(pTokens);
    if(pTokens->peekToken()->isOperatorTermination())
      return;
  }
  else if(pTok->isOperatorTypeCastLeftParen())
    parseTypeSizeCast(pTokens);
  else if(pTok->isSymbolTyped())
    parseType(pTokens);
  else if(pTok->isOperatorAs())
  {
    parseElementAs(pTokens);
    return;
  }
  else if(pTok->isElementEnd())
  {
    parseElementEnd(pTokens);
    return;
  }
  else
    Utils::Logger::logErr(pTok, "Expected sub-expression, got \"" + pTok->text + "\"");
  if(m_pElement->isElementEnded()) return;
  pTokens->nextToken();
}

void GrammarElementParser::parseExprTail(std::shared_ptr<Tokens> pTokens)
{
  parseSubExpr(pTokens);

  auto pTok = pTokens->peekToken();
  auto pLastTok = pTokens->peekToken(1, Tokens::BACKWARD);
  if(pTok->isOperatorConcat() || pTok->isOperatorOr())
  {
    pTokens->nextToken();
    parseExpr(pTokens);
  }
  else if(pTok->isOperatorTermination())
  {
    auto pNextTok = pTokens->nextToken();
    if(pNextTok->isElementEnd())
    {
      parseElementEnd(pTokens);
      return;
    }
    parseExpr(pTokens);
  }
  else if(pTok->isOperatorLeftRangeBracket())
  {
    pTokens->nextToken();
    parseRange(pTokens);
    pTokens->nextToken();
    parseExpr(pTokens);
  }
  else if(pTok->isElementEnd())
  {
    parseElementEnd(pTokens);
    return;
  }
  else
    Utils::Logger::logErr(pTok, "Expected one of these operators: ',', '|', ';', '{', or element END, got \"" + pTok->text + "\"");
}

void GrammarElementParser::parseNonTerm(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  auto pNextTok = pTokens->peekToken(1, Tokens::FORWARD);
  if(!pTok->isOperatorAssignment() && pNextTok && pNextTok->isOperatorAssignment())
    pTok = pTokens->nextToken();
  if(pTok->isOperatorAssignment())
  {
    assignTokTypeBeforeTypeSizeCast = pTok->type;
    auto pLastToken = pTokens->lastToken();
    if(pLastToken)
    {
      if(pLastToken->isSymbolGrammar())
        parseNonTermSymbol(pTokens);
      else if(pNextTok->isSymbolGrammarSize())
        m_pAssignedSubElement.reset(); //TRICKY: no assigned subelement since the grammar size applies to the element.
      else if(pLastToken->isStaticVariable())
        parseNonTermStaticVar(pTokens);
      else
        Utils::Logger::logErr(pLastToken, "Expected symbol or static variable, got \"" + pLastToken->text + "\"");
    }
    else
      Utils::Logger::logErr("Expected symbol or static variable");
    pTokens->nextToken();
  }
}

void GrammarElementParser::parseLiteral(std::shared_ptr<Tokens> pTokens)
{
  addCurrentTokenToAssignedSubElement(pTokens);
}

void GrammarElementParser::parseNonTermSymbol(std::shared_ptr<Tokens> pTokens)
{
  addAssignedSubElementToElement(pTokens);
}

void GrammarElementParser::parseSubExprSymbol(std::shared_ptr<Tokens> pTokens)
{
  addCurrentTokenToAssignedSubElement(pTokens);
}

void GrammarElementParser::parseNonTermStaticVar(std::shared_ptr<Tokens> pTokens)
{
  //addAssignedSubElementToElement(pTokens);
  std::vector<std::shared_ptr<Token> > rpnTokens;
  auto pLastTok = pTokens->lastToken();
  if(pLastTok->isStaticVariable() && pTokens->peekToken()->isOperatorAssignment())
    rpnTokens.push_back(pLastTok);
  auto exprTokens = parseBasicExpr(pTokens, Token::OPERATOR_TERMINATION, false);
  rpnTokens.insert(rpnTokens.end(), exprTokens.begin(), exprTokens.end());
  auto pActionLineSe = std::make_shared<SubElementActionLine>(rpnTokens);
  m_pElement->getChildren()->addChild(pActionLineSe);
}

void GrammarElementParser::parseSubExprStaticVar(std::shared_ptr<Tokens> pTokens)
{
  //addCurrentTokenToAssignedSubElement(pTokens);
  parseNonTermStaticVar(pTokens);
}

void GrammarElementParser::parseType(std::shared_ptr<Tokens> pTokens)
{
  addCurrentTokenToAssignedSubElement(pTokens);
}

void GrammarElementParser::parseTypeSizeCast(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->nextToken();
  if(!pTok->isSymbolTyped())
    Utils::Logger::logErr(pTok, "Expected type, got \"" + pTok->text + "\"");
  auto pSubElement = std::make_shared<SubElement>(pTok);
  pSubElement->setCppTypeInfo(TypeUtils::getCppTypeInfo(pSubElement.get(), pSubElement.get()));
  pTypeSizeCastSubElement = pSubElement;
  pTok = pTokens->nextToken();
  if(pTok->isOperatorLeftRangeBracket())
  {
    pTokens->nextToken();
    parseRange(pTokens);
    pTok = pTokens->nextToken();
    parseExpr(pTokens);
  }
}

void GrammarElementParser::parseRange(std::shared_ptr<Tokens> pTokens)
{
  std::shared_ptr<Token> pTok = pTokens->peekToken();
  size_t startIdx = pTok->index;
  parseRangeSubExpr(pTokens);
  pTok = pTokens->peekToken();
  if(pTok->isOperatorRightRangeBracket())
  {
    size_t endIdx = pTok->index;
    auto rangeTokens = pTokens->getTokenRange(startIdx, endIdx + 1);
    if(!rangeTokens.empty())
    {
      auto pTypeSubElement = pTypeSizeCastSubElement ? pTypeSizeCastSubElement :
        m_pAssignedSubElement->getChildren()->getLastChild();
      if(!pTypeSubElement->getCppTypeInfo())
      {
        pTypeSubElement->setCppTypeInfo(
          TypeUtils::getCppTypeInfo(pTypeSubElement.get(), pTypeSubElement.get()));
      }
      pTypeSubElement->getCppTypeInfo()->pTypeRange =
        std::make_shared<TypeRange>(m_pAssignedSubElement, rangeTokens);
      size_t sz = pTypeSubElement->getCppTypeInfo()->pTypeRange->getSize();
      if("std::bitset" == pTypeSubElement->getCppTypeInfo()->typeStr)
      {
        size_t bitSize = pTypeSubElement->getCppTypeInfo()->pTypeRange->getSize();
        pTypeSubElement->getCppTypeInfo()->resolvedSizeBytes =
          (bitSize % CHAR_BIT > 0) ? ((bitSize / CHAR_BIT) + 1) : (bitSize / CHAR_BIT);
        pTypeSubElement->getCppTypeInfo()->resolvedSizeBits = bitSize;
      }
      else
      {
        pTypeSubElement->getCppTypeInfo()->resolvedSizeBytes =
          sz * (pTypeSubElement->getCppTypeInfo()->typeSizeBits/CHAR_BIT);
      }
      
      if(!pTypeSizeCastSubElement &&
         m_pAssignedSubElement->getCppTypeInfo() &&
         m_pAssignedSubElement->getCppTypeInfo()->typeNameStr ==
           pTypeSubElement->getCppTypeInfo()->typeNameStr)
      {
        m_pAssignedSubElement->setCppTypeInfo(pTypeSubElement->getCppTypeInfo());
      }

      auto pGrammarElement = std::dynamic_pointer_cast<GrammarElement>(m_pElement);
      if(pTypeSubElement->getCppTypeInfo() &&
         pTypeSubElement->getCppTypeInfo()->pTypeRange &&
         pTypeSubElement->getCppTypeInfo()->pTypeRange->isVectorUnbounded())
      {
        pGrammarElement->setLastTerminalSubElement(m_pAssignedSubElement);
      }
      else if(pGrammarElement->getLastTerminalSubElement())
      {
        pGrammarElement->setLastTerminalSubElement(std::shared_ptr<SubElement>());
      }
    }
  }
  pTokens->nextToken();
}

void GrammarElementParser::parseRangeSubExpr(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(pTok->isRangeValue() ||
     pTok->isStaticVariable() ||
     pTok->isSymbolGrammar())
  {
    pTokens->nextToken();
    parseRangeExprTail(pTokens);
  }
  else
    parseRangeExprTail(pTokens);
}

void GrammarElementParser::parseRangeExprTail(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(pTok->isOperatorRangeDelim())
  {
    pTokens->nextToken();
    parseRangeSubExpr(pTokens);
  }
  else if(!pTok->isOperatorRightRangeBracket())
    Utils::Logger::logErr(pTok, "Invalid range expression");
}

