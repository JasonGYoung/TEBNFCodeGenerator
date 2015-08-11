
//Primary include:
#include "StateTableElementParser.hpp"

//Local includes:
#include "../Utils/Utils.hpp"

namespace
{
  auto currentStep = SubElementState::STATE;
  std::shared_ptr<SubElementState> pCurrentStateSubElement;
  size_t elseIfCounter = 1;
}

void StateTableElementParser::parse(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  auto pNextTok = pTokens->nextToken();
  if(pTok->isElementStateTable() && pNextTok->isElementName())
    parseStep(pTokens);
  else
    Utils::Logger::logErr(pTok, "State table expected");
}

void StateTableElementParser::parseStep(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->nextToken();
  if(!pTok)
  {
    parseElementEnd(pTokens);
  }
  if(pTok->isOperatorStateTableDelimiter() || pTok->isOperatorTermination())
  {
    if((pTok->isOperatorStateTableDelimiter() || 
        pTok->isOperatorTermination()) &&
       pTokens->peekToken(1, Tokens::BACKWARD)->isOperatorStateTableDelimiter())
    {
      //Skip to next step if possible.
      switch(currentStep)
      {
      case SubElementState::STATE: parseStepStateElse(pTokens); break;
      case SubElementState::INPUT_OR_CONDITION: currentStep = SubElementState::INPUT_METHOD; break;
      case SubElementState::INPUT_METHOD:  currentStep = SubElementState::NEXT_STATE; break;
      case SubElementState::NEXT_STATE: currentStep = SubElementState::OUTPUT_OR_ACTION; break;
      case SubElementState::OUTPUT_OR_ACTION: currentStep = SubElementState::OUTPUT_METHOD; break;
      case SubElementState::OUTPUT_METHOD: currentStep = SubElementState::STATE; break;
      };
    }
    else if(pTok->isOperatorStateTableDelimiter() &&
            pTokens->lastToken()->isOperatorTermination() &&
            SubElementState::STATE == currentStep)
    {
      parseStepStateElse(pTokens);
    }
    parseStep(pTokens);
  }
  else if(pTok->isElementEnd())
    parseElementEnd(pTokens);
  else
  {
    auto pCurTok = pTokens->peekToken();
    auto pLastTok = pTokens->peekToken(1, Tokens::BACKWARD);
    auto pLastLastTok = pTokens->peekToken(2, Tokens::BACKWARD);
    if((pLastLastTok->isElement() && 
        pLastTok->isElementName()) ||
       pLastTok->isOperatorStateTableDelimiter() ||
       pLastTok->isOperatorTermination())
    {
      switch(currentStep)
      {
      case SubElementState::STATE: parseStepState(pTokens); break;
      case SubElementState::INPUT_OR_CONDITION: parseStepInputOrCond(pTokens); break;
      case SubElementState::INPUT_METHOD: parseStepInputMethod(pTokens); break;
      case SubElementState::NEXT_STATE: parseStepNextState(pTokens); break;
      case SubElementState::OUTPUT_OR_ACTION: parseStepOutputOrAction(pTokens); break;
      case SubElementState::OUTPUT_METHOD: parseStepOutputMethod(pTokens); break;
      default: break;
      };
      parseStep(pTokens);
    }
    else
      Utils::Logger::logErr(pTokens->peekToken(1, Tokens::BACKWARD), "Invalid state table step");
  }
}

namespace
{
  std::string currentState;
}

void StateTableElementParser::parseStepState(std::shared_ptr<Tokens> pTokens)
{
  elseIfCounter = 1;
  currentState = pTokens->peekToken()->text;
  pCurrentStateSubElement = std::make_shared<SubElementState>(pTokens->peekToken());
  m_pElement->getChildren()->addChild(pCurrentStateSubElement);
  currentStep = SubElementState::INPUT_OR_CONDITION;
}

void StateTableElementParser::parseStepStateElse(std::shared_ptr<Tokens> pTokens)
{
  std::stringstream elseTokName;
  if(pTokens->peekToken()->isOperatorStateTableDelimiter() &&
     pTokens->peekToken(1, Tokens::FORWARD)->isOperatorStateTableDelimiter())
  {
    elseTokName << currentState << "_else";
  }
  else
  {
    elseTokName << currentState << "_else_if_" << elseIfCounter;
  }
  elseIfCounter++;
  auto pToken = std::make_shared<Token>(Token::SYMBOL_STATE_TABLE_STATE, elseTokName.str());
  pCurrentStateSubElement = std::make_shared<SubElementState>(pToken);
  m_pElement->getChildren()->addChild(pCurrentStateSubElement);
  currentStep = SubElementState::INPUT_OR_CONDITION;
}

void StateTableElementParser::parseStepInputOrCond(std::shared_ptr<Tokens> pTokens)
{
  //The contents of this field must evaluate to a boolean expression, which
  //could be:
  //1) A grammar element or subelement (meaning that incoming data matched).
  //2) A console subelement (meaning that the user provided input.
  //3) A boolean expression.
  //4) (TODO) An external library API call.
  auto rpnTokens = parseBasicExpr(pTokens, Token::OPERATOR_STATE_TABLE_DELIM, false);
  if(1 == rpnTokens.size())
  {
    std::shared_ptr<SubElement> pSubElem;
    auto pTok = rpnTokens[0];
    if(!pTok->isElementName())
    {
      if(pTok->pAccessedElementToken && (pTok->isSymbolStateTable() || pTok->isTypeUnknownOrNone()))
      {
        pSubElem = Elements::findSubElement(pTok);
        if(pSubElem && !pSubElem->getToken()->isTypeUnknownOrNone())
          pTok->type = pSubElem->getToken()->type;
      }
      if(!pSubElem)
        Utils::Logger::logErr(pTok, "Need a boolean condition, grammar element, grammar subelement, or console subelement as input");
    }
    if(pSubElem)
    {
      pCurrentStateSubElement->setInputOrCondition(pSubElem);
    }
    else
    {
      auto pInputOrCondElement = Elements::findElement(pTok->text);
      if(!pInputOrCondElement ||
         (!pInputOrCondElement->getToken()->isElementGrammar() &&
          !pInputOrCondElement->getToken()->isElementInput()))
      {
        Utils::Logger::logErr(pTok, "Need a boolean condition, grammar element, grammar subelement, or console subelement as input");
      }
      pCurrentStateSubElement->setInputOrCondition(pInputOrCondElement);
    }
  } 
  else
  {
    auto pActionLineSe = std::make_shared<SubElementActionLine>(rpnTokens);
    pCurrentStateSubElement->setInputOrCondition(pActionLineSe);    
  }
  currentStep = SubElementState::INPUT_METHOD;
}

void StateTableElementParser::parseStepInputMethod(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(pTok->isElementName())
  {
    auto pElem = Elements::findElement(pTok->text);
    if(pElem && pElem->getToken()->isElementInput())
    {
      pCurrentStateSubElement->setInputMethod(pElem);
      currentStep = SubElementState::NEXT_STATE;
    }
  }
  else if(pTok->pAccessedElementToken)
  {
    auto pSubElem = Elements::findSubElement(pTok);
    if(pSubElem && pTok->pAccessedElementToken)
    {
      auto pIoSubElement = std::make_shared<SubElement>(pTok);
      pIoSubElement->setCppTypeInfo(pSubElem->getCppTypeInfo());
      pCurrentStateSubElement->setInputMethod(pIoSubElement);
      currentStep = SubElementState::NEXT_STATE;
    }
  }
  if(SubElementState::NEXT_STATE != currentStep)
    Utils::Logger::logErr(pTok, "Expected input element or subelement");
}

void StateTableElementParser::parseStepNextState(std::shared_ptr<Tokens> pTokens)
{
  pCurrentStateSubElement->setNextState(pTokens->peekToken()->text);
  currentStep = SubElementState::OUTPUT_OR_ACTION;
}

void StateTableElementParser::parseStepOutputOrAction(std::shared_ptr<Tokens> pTokens)
{
  auto pNextTok = pTokens->peekToken(1, Tokens::FORWARD);
  if(pNextTok->isOperatorStateTableDelimiter())
  {
    auto pTok = pTokens->peekToken();
    std::shared_ptr<Node> pOutputOrActionNode;
    if(pTok->isElementActions())
      pOutputOrActionNode = Elements::findElement(pTok->text);
    else if(pTok->isElementName())
    {
      auto pElement = Elements::findElement(pTok->text);
      if(pElement &&
         (pElement->getToken()->isElementGrammar() ||
          pElement->getToken()->isElementActions()))
        pOutputOrActionNode = pElement;
    }
    else if(pTok->pAccessedElementToken)
    {
      auto pElem = Elements::findElement(pTok->pAccessedElementToken->text);
      pOutputOrActionNode = pElem->getChildren()->findChild(pTok->text);
      if(!pOutputOrActionNode)
        Utils::Logger::logErr(pTok->lineNumber, "Element \"" + pElem->getName() + "\" has no member named \"" + pTok->text + "\"");
      pOutputOrActionNode->setToken(pTok);
    }
    if(pOutputOrActionNode)
      pCurrentStateSubElement->setOutputOrAction(pOutputOrActionNode);
    else
      Utils::Logger::logErr(pTok, "Must provide valid output or action(s)");
  }
  else
  {
    auto rpnTokens = parseBasicExpr(pTokens, Token::OPERATOR_STATE_TABLE_DELIM, false);
    if(!rpnTokens.empty() && rpnTokens.at(0)->isElementName())
    {
      auto pFoundElem = Elements::findElement(rpnTokens.at(0)->text);
      if(pFoundElem && pFoundElem->isType("ActionsElement"))
      {
        auto pNewFoundElem = std::dynamic_pointer_cast<ActionsElement>(pFoundElem);
        auto pActionsElem = std::make_shared<ActionsElement>(*pNewFoundElem);
        for(std::shared_ptr<Token> pRpnTok : rpnTokens)
        {
          if(pRpnTok->isLiteral() ||
             pRpnTok->isSymbolGrammar() ||
             pRpnTok->isStaticVariable() ||
             pRpnTok->isSymbolActions())
            pActionsElem->addArg(pRpnTok);
        }
        pCurrentStateSubElement->setOutputOrAction(pActionsElem);
        // TRICKY KLUDGE: We need a deep copy of this object because args
        // are different per call in the table, but this also means the
        // flag that determines if this element is used in the state table
        // only gets set in the copy.  This means it was not getting generated.
        pFoundElem->setIsUsedInStateTable(pActionsElem->isUsedInStateTable());
      }
    }
    if(!pCurrentStateSubElement->getOutputOrAction())
    {
      auto pActionLineSe = std::make_shared<SubElementActionLine>(rpnTokens);
      pCurrentStateSubElement->setOutputOrAction(pActionLineSe);
    }
  }
  currentStep = SubElementState::OUTPUT_METHOD;
}

void StateTableElementParser::parseStepOutputMethod(std::shared_ptr<Tokens> pTokens)
{
  auto pTok = pTokens->peekToken();
  if(!pTok->isElementName())
    Utils::Logger::logErr(pTok, "Output element needs a name");
  auto pElem = Elements::findElement(pTok->text);
  if(pElem && pElem->getToken()->isElementOutput())
  {
    pCurrentStateSubElement->setOutputMethod(pElem);
    currentStep = SubElementState::STATE; //Start on next state.
  }
  else
    Utils::Logger::logErr(pTok, "Expected output element");
}
