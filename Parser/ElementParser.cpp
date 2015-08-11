//Primary include:
#include "ElementParser.hpp"

#include "../Utils/Utils.hpp"

#include <list>
#include <stack>
#include <string>

std::vector<std::shared_ptr<Token> >  ElementParser::parseBasicExpr(
  std::shared_ptr<Tokens> pTokens,
  Utils::optional<Token::TokenType> optEndTokType,
  bool returnTokensInRpn)
{
  std::vector<std::shared_ptr<Token> > regOrderList;
  std::list<std::shared_ptr<Token> > outputQueue;
  std::stack<std::shared_ptr<Token> > tokenStack;
  bool isInsideArrayBrackets = false;
  auto pTok = pTokens->peekToken();
  while(pTok)
  {
    pTok = pTokens->peekToken();
    if(optEndTokType && pTok->type == *optEndTokType)
      break; //If we've reached the end token, quit now.
    else if(pTok->isOperatorLeftParen())
      tokenStack.push(pTok);
    else if(pTok->isOperatorRightParen())
    {
      auto pTopToken = tokenStack.top();
      while(!pTopToken->isOperatorLeftParen())
      {
        outputQueue.push_back(pTopToken);
        tokenStack.pop();
        if(tokenStack.empty())
          break;
        pTopToken = tokenStack.top();
      }
      if(!tokenStack.empty())
        tokenStack.pop();
      if(!pTopToken->isOperatorLeftParen())
        Utils::Logger::logErr(pTopToken, "Mismatched parentheses");
    }
    else if(pTok->isOperatorMath() || pTok->isOperatorComparison())
    {
      auto pTokOp1 = pTok;
      if(!tokenStack.empty())
      {
        auto pTokOp2 = tokenStack.top();
        while(pTokOp2->isOperator() &&
              ((pTokOp1->isLeftAssociative() && Token::cmpPrecedence(*pTokOp1, *pTokOp2) == 0) ||
               (Token::cmpPrecedence(*pTokOp1, *pTokOp2) < 0)))
        {
          tokenStack.pop();
          outputQueue.push_back(pTokOp2);
          if(tokenStack.empty()) break;
          pTokOp2 = tokenStack.top();
        }
      }
      tokenStack.push(pTokOp1);
    }
    else if(pTok->isOperatorLeftArrayBracket())
      isInsideArrayBrackets = true;
    else if(isInsideArrayBrackets)
    {
      if(pTok->isOperatorRightArrayBracket())
        isInsideArrayBrackets = false;
    }
    else
      outputQueue.push_back(pTok);
    regOrderList.push_back(pTok);
    pTok = pTokens->nextToken();
  }

  if(!returnTokensInRpn)
    return regOrderList;

  while(!tokenStack.empty())
  {
    const auto pStackTok = tokenStack.top();
    if(pStackTok->isOperatorLeftParen() || pStackTok->isOperatorRightParen())
      Utils::Logger::logErr(pStackTok, "Mismatched parentheses");
    outputQueue.push_back(pStackTok);
    tokenStack.pop();
  }          

  std::vector<std::shared_ptr<Token> > ret(outputQueue.begin(), outputQueue.end());    
  return ret;
}

void ElementParser::parseElementAs(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(!pTok->isOperatorAs())
    Utils::Logger::logErr(pTok, "Expected \"" + pTok->text + "\"");
  auto pNextTok = pTokens->nextToken();
  auto pElem = Elements::findElement(pNextTok->text);
  if(!pElem)
    Utils::Logger::logErr(pNextTok, "A valid element must follow \"" + pTok->text + "\"");
  if(!pNextTok->isElementName())
    Utils::Logger::logErr(pNextTok, "Element name must follow \"" + pTok->text + "\"");
  m_pElement->setElementType(pElem->getElementType());
  m_pElement->getChildren()->pChildElement = pElem;
  m_pElement->setAsElement(true);
  pTokens->nextToken();
  parseElementEnd(pTokens);
}

void ElementParser::parseElementEnd(std::shared_ptr<Tokens> pTokens)
{
  if(!m_pElement->isElementEnded())
    m_pElement->setIsElementEnded(isElementEnded(pTokens));
  if(!m_pElement->isElementEnded())
  {
    // TRICKY: Log using last token since the current token is probably NULL in this case.
    Utils::Logger::logErr(pTokens->lastToken(), "Element declaration not ended");
  }
}

bool ElementParser::isElementEnded(std::shared_ptr<Tokens> pTokens)
{
  return m_pElement && pTokens->peekToken() && pTokens->peekToken()->isElementEnd();
}
