
//Primary include:
#include "IoElementParser.hpp"

//Local includes:
#include "../Utils/Utils.hpp"

void IoElementParser::parse(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(!pTok->isElementIo())
    Utils::Logger::logErr(pTok, "Expected IO element");
  pTok = pTokens->nextToken();
  if(!pTok->isElementName())
    Utils::Logger::logErr(pTok, "Invalid element name");
  pTokens->nextToken();
  parseExprTail(pTokens);
}

void IoElementParser::parseExpr(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(!pTok->isOperatorTermination() && !pTok->isElementEnd())
    Utils::Logger::logErr(pTok, "Invalid expression");
  parseElementEnd(pTokens);
}

void IoElementParser::parseExprTail(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(pTok->isOperatorAs())
    parseElementAs(pTokens);
  else
  {
    if(!pTok->isOperatorAssignment(true))
      Utils::Logger::logErr(pTok, "Expected IO type assignment or AS");
    pTok = pTokens->nextToken();    
    if(pTok->isSymbolIoType() || pTok->isElementName())
    {
      setElemType(pTok);
      pTok = pTokens->nextToken();
      if(pTok->isSymbolConsole())
        parseExprTerm(pTokens);
      else
        parseExpr(pTokens);
    }
    else
      Utils::Logger::logErr(pTok, "Expected element name or IO type");
  }
}

void IoElementParser::parseExprTerm(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  m_pAssignedSubElement = std::make_shared<SubElement>(pTok);
  m_pElement->getChildren()->addChild(m_pAssignedSubElement);
  pTok = pTokens->nextToken();
  if(!pTok->isOperatorAssignment())
    Utils::Logger::logErr(pTok, "Expected assignment");
  pTok = pTokens->nextToken();
  if(!pTok->isSymbolTyped())
    Utils::Logger::logErr(pTok, "Expected type");
  std::shared_ptr<CppTypeInfo> pTypeCppInfo;
  if(TypeUtils::getCppTypeInfo(pTok, pTypeCppInfo))
  {
    auto pTypedSubElem = std::make_shared<SubElement>(pTok);
    pTypedSubElem->setCppTypeInfo(pTypeCppInfo);
    m_pAssignedSubElement->getChildren()->addChild(pTypedSubElem);
  }
  pTok = pTokens->nextToken();
  if(!pTok->isOperatorAssignment(true))
    Utils::Logger::logErr(pTok, "Expected assignement");
  pTok = pTokens->nextToken();
  if(!pTok->isLiteralString())
    Utils::Logger::logErr(pTok, "Expected string");
  auto pStringSubElem = std::make_shared<SubElement>(pTok);
  m_pAssignedSubElement->getChildren()->addChild(pStringSubElem);
  pTok = pTokens->nextToken();
  if(!pTok->isOperatorTermination())
    Utils::Logger::logErr(pTok, "Assigned console string is not terminated");
  pTok = pTokens->nextToken();
  if(!pTok->isElementEnd())
    parseExprTerm(pTokens);
  parseExpr(pTokens);
}

void IoElementParser::setElemType(std::shared_ptr<Token> pTok)
{
  if(pTok->isSymbolIoType())
    m_pElement->setElementType(Types::getElementType(pTok, m_pElement));
  else if(pTok->isElementName())
  {
    auto pElem = Elements::findElement(pTok->text);
    if(pElem)
    {
      m_pElement->setElementType(pElem->getElementType());
      m_pElement->getChildren()->pChildElement = pElem;
    }
  }
}
