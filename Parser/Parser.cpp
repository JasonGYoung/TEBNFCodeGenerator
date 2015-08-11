/**
 *  TEBNF Parser.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-01-03
 * @see header file for description.
 */

//Primary include:
#include "Parser.hpp"

#include "ActionsElementParser.hpp"
#include "GrammarElementParser.hpp"
#include "IoElementParser.hpp"
#include "StateTableElementParser.hpp"

//System includes:
#include <algorithm>
#include <deque>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <typeinfo>

namespace
{
  void countBraces(const std::vector<std::shared_ptr<Token> >& tokens,
                   size_t curIdx)
  {
    static int brackCnt = 0;
    static int parenCnt = 0;
    static int sqlBrackCnt = 0;
    static std::shared_ptr<Token> pBrackToken;
    static std::shared_ptr<Token> pParenToken;
    static std::shared_ptr<Token> pSqrBrackToken;
    auto pToken = Utils::getItemAt(tokens, curIdx);
    if(tokens.size()-1 == curIdx ||
       pToken->isOperatorTermination() ||
       pToken->isElementEnd())
    {
      if(brackCnt != 0)
        Utils::Logger::logErr(pBrackToken, "Mismatched brackets");
      if(parenCnt != 0)
        Utils::Logger::logErr(pParenToken, "Mismatched parenthesis");
      if(sqlBrackCnt != 0)
        Utils::Logger::logErr(pSqrBrackToken, "Mismatched array subscript bracket");
      //Reset counts in case we've only reached the end of the element.
      brackCnt = 0;
      parenCnt = 0;
      sqlBrackCnt = 0;
      pBrackToken.reset();
      pParenToken.reset();
      pSqrBrackToken.reset();
      return;
    }
    //Mismatched brackets
    if("{" == pToken->text)
    {
      pBrackToken = pToken;
      brackCnt++;
    }
    else if("}" == pToken->text)
    {
      pBrackToken = pToken;
      brackCnt--;
    }
    if(brackCnt < 0)
      Utils::Logger::logErr(pBrackToken, "Mismatched brackets");
    //Mismatched parenthesis
    if("(" == pToken->text)
    {
      pParenToken = pToken;
      parenCnt++;
    }
    else if(")" == pToken->text)
    {
      pParenToken = pToken;
      parenCnt--;
    }
    if(parenCnt < 0)
      Utils::Logger::logErr(pParenToken, "Mismatched parenthesis");
    //Mismatched array subscript bracket
    if("[" == pToken->text)
    {
      pSqrBrackToken = pToken;
      sqlBrackCnt++;
    }
    else if("]" == pToken->text)
    {
      pSqrBrackToken = pToken;
      sqlBrackCnt--;
    }
    if(sqlBrackCnt < 0)
      Utils::Logger::logErr(pSqrBrackToken, "Mismatched array subscript bracket");
  }

}

void Parser::parseTokens()
{
  Utils::Logger::log(Utils::getTabSpace() + "Parsing...", false);
  auto pTokens = Scanner::getTokens();
  std::shared_ptr<SubElement> pAssignedSubElement;
  for(auto pTok = pTokens->peekToken(); pTok; pTok = pTokens->nextToken())
  {
    if(parseNewElement(m_pCurrentElement, pTokens))
    {
      if(pTok->isElementGrammar())
      {
        GrammarElementParser gp(m_pCurrentElement);
        gp.parse(pTokens);
      }
      else if(pTok->isElementIo())
      {
        IoElementParser iep(m_pCurrentElement);
        iep.parse(pTokens);
      }
      else if(pTok->isElementActions())
      {
        ActionsElementParser aep(m_pCurrentElement);
        aep.parse(pTokens);
      }
      else if(pTok->isElementStateTable())
      {
        StateTableElementParser ste(m_pCurrentElement);
        ste.parse(pTokens);
      }
      validateNewElement(m_pCurrentElement);
    }
  }

  //Resolve elements and subelements.
  for(std::shared_ptr<Element> pElement : Elements::elements())
    pElement->resolve();

  //Report any unresolved elements and subelements.
  for(std::shared_ptr<Element> pElement : Elements::elements())
    pElement->logUnresolved();

  Utils::Logger::log(" finished");
}

bool Parser::validateNewElement(std::shared_ptr<Element>& pElement)
{
  if(pElement && pElement->isElementEnded() && !Elements::findElement(pElement->getName()))
  {
    Elements::elements().push_back(pElement);
    Elements::elementTable()[pElement->getName()] = pElement;
    pElement.reset();
    return true;
  }
  return false;
}

bool Parser::parseNewElement(std::shared_ptr<Element>& pElement, std::shared_ptr<Tokens> pTokens)
{
  if(!pElement && pTokens->peekToken()->isElement())
  {
    //New element.
    auto pElementToken = pTokens->peekToken();
    if(!pElementToken->isElement())
      Utils::Logger::logErr(pElementToken, "Expected element");
    auto pElementNameToken = pTokens->peekToken(1, Tokens::FORWARD);
    if(!pElementNameToken->isElementName())
      Utils::Logger::logErr(pElementNameToken, "Element must have a name");
    pElement = Elements::getNewElementInstance(pElementToken);
    pElement->setName(pElementNameToken->text);
    pElement->setCppTypeInfo(std::make_shared<CppTypeInfo>());
    pElement->getCppTypeInfo()->typeStr = "Element";
    pElement->getCppTypeInfo()->typeNameStr = Utils::getCppVarName(pElementNameToken->text);
    if(Elements::findElement(pElement->getName()))
      Utils::Logger::logErr(pElementNameToken, "Element redeclaration");
    return true;
  }
  else if(pElement && !pElement->isElementEnded())
    Utils::Logger::logErr(pElement->getToken(), "Element missing END");
  return false;
}

