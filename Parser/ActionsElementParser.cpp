
//Primary include:
#include "ActionsElementParser.hpp"

//System includes:
#include <queue>
#include <stack>

void ActionsElementParser::parse(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(pTok->isElementActions())
  {
    pTok = pTokens->nextToken();
    if(!pTok->isElementName())
      Utils::Logger::logErr(pTok, "Expected element name");
    pTokens->nextToken();
  }
  parseParams(pTokens);
  parseExpr(pTokens);
}

void ActionsElementParser::parseExpr(std::shared_ptr<Tokens> pTokens)
{
  auto rpnTokens = parseBasicExpr(pTokens, Token::OPERATOR_TERMINATION, false);
  auto pActionLineSe = std::make_shared<SubElementActionLine>(rpnTokens);
  m_pElement->getChildren()->addChild(pActionLineSe);
  auto pTok = pTokens->nextToken();
  if(pTok->isElementEnd())
    parseElementEnd(pTokens);
  else
    parseExpr(pTokens);
}

void ActionsElementParser::parseParams(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(pTok->isSymbolActionsParam())
  {
    auto pActionsElement = std::dynamic_pointer_cast<ActionsElement>(m_pElement);
    pActionsElement->addParam(pTok);
    pTok = pTokens->nextToken();
    parseParams(pTokens);
  }
  else if(pTok->isOperatorLeftParen() || pTok->isOperatorActionsParamDelim())
  {
    pTok = pTokens->nextToken();
    parseParams(pTokens);
  }
  else if(pTok->isActionsLastSignatureToken)
    pTok = pTokens->nextToken();
  else if(!pTokens->peekToken(1, Tokens::BACKWARD)->isActionsLastSignatureToken)
    Utils::Logger::logErr(pTok, "Invalid actions parameter");
}
