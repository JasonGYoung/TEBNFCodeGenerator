
#include "StateTableElement.hpp"

#include "Elements.hpp"
#include "ScopedBlock.hpp"

namespace
{
  std::string getInputReadFuncCall(SubElementState* pStateSubElement)
  {
    std::string inputReadFuncCall;
    auto pInputElement = pStateSubElement->getInputElement();
    if(pInputElement && pInputElement->getCppTypeInfo())
    {
      std::string inputElementAccessed(pInputElement->getNodeAccessor());
      if(pInputElement)
      {
        switch(pInputElement->getElementType())
        {
        case Types::ELEMENT_TYPE_IO_INPUT_UDP_IP:
          {
            inputReadFuncCall = "if(" + inputElementAccessed + "read())";
          }
          break;
        case Types::ELEMENT_TYPE_IO_INPUT_FILE:
          {
            inputReadFuncCall = "if(" + inputElementAccessed + "read())";
          }
          break;
        case Types::ELEMENT_TYPE_IO_INPUT_CONSOLE:
          if(pInputElement->hasChildren())
          {
            auto pInputSe = pInputElement->getChildren()->findChild(pStateSubElement->getInputMethod()->getToken()->text);
            if(pInputSe && pInputSe->getChildren()->children.size() == 2)
            {
              auto pTypeChild = pInputSe->getChildren()->getChild(0);
              auto pStringChild = pInputSe->getChildren()->getChild(1);
              inputReadFuncCall = "if(" + inputElementAccessed + "read<" + pTypeChild->getCppTypeInfo()->typeStr + ">(" + pStringChild->getToken()->text + "))";
            }
          }
          break;
        };
      }
    }
    return inputReadFuncCall;
  }

  template<typename STREAM_T>
  void generateSubElementActionLine(STREAM_T& block, Node* pNodeSubElementActionLine)
  {
    if(pNodeSubElementActionLine->isType("SubElementActionLine"))
    {
      auto pSubElementActionLine = reinterpret_cast<SubElementActionLine*>(pNodeSubElementActionLine);
      auto rpnTokens = pSubElementActionLine->getRpnTokens();
      std::string condStr;
      for(auto it = rpnTokens.begin(); it != rpnTokens.end(); ++it)
      {
        if(!condStr.empty())
          condStr += " ";
        condStr += Utils::getCppVarName(*it);
      }
      block << condStr;
    }
  }

  void generateNextStateAndOutput(
    ScopedBlock& block,
    SubElementState* pStateSubElement,
    const std::string& nextStateCleanup = "")
  {
    auto pOutputOrAction = pStateSubElement->getOutputOrAction();
    auto pOutputElement = pStateSubElement->getOutputElement();
    std::string nextState = pStateSubElement->getNextState();
    if(nextState.empty())
      block << "m_isRunning = false; // Exit state machine\n";
    else
    {
      block << "m_state = STATES_" << nextState << ";\n";
      if(!nextStateCleanup.empty())
        block << nextStateCleanup;
    }
    if(pOutputOrAction)
    {
      if(!pOutputOrAction->getToken()->isSymbolActions() &&
         !pOutputOrAction->getToken()->isSymbolGrammar() &&
         !pOutputOrAction->getToken()->isStaticVariable() &&
         pOutputOrAction->isType("SubElementActionLine"))
      {
        generateSubElementActionLine(block, pOutputOrAction.get());
        block << ";\n";
      }
      else if("ActionsElement" == pOutputOrAction->getTypeName())
      {
        auto pActionsElem = reinterpret_cast<ActionsElement*>(pOutputOrAction.get());
        block << pActionsElem->getActionsFunctionCall() << ";\n";
      }       
      else if(pOutputElement)
      {
        if(Types::ELEMENT_TYPE_IO_OUTPUT_CONSOLE == pOutputElement->getElementType())
        {
          if(pOutputOrAction->getToken()->isSymbolActions() ||
             pOutputOrAction->getToken()->isSymbolGrammar() ||
             pOutputOrAction->getToken()->isStaticVariable())
          {
            std::string elementAccessor = Utils::getElementAccessor(pOutputElement);
            std::string varName = Utils::getCppVarName(pOutputOrAction->getToken());
            block << elementAccessor << "write(" << varName << ");\n";
          }
        }
        else
        {
          auto pOutputOrActionCppInfo = pOutputOrAction->getCppTypeInfo();
          auto pOutputElementCppInfo = pOutputElement->getCppTypeInfo();
          if(pOutputOrActionCppInfo && pOutputElementCppInfo)
          {
            std::string elementAccessor = Utils::getElementAccessor(pOutputElement);
            std::string outputOrActionAccessor = Utils::getElementAccessor(*pOutputOrActionCppInfo);
            block << "std::vector<uint8_t> marshalledData;\n";
            block << pOutputOrActionCppInfo->getMarshalCall(outputOrActionAccessor, "marshalledData") + ";\n";
            block << elementAccessor << "write(&marshalledData[0], marshalledData.size());\n";
          }
        }
      }
    }
  }

  void generateInputOrCondElseIfBlocks(
    std::stringstream& line,
    const std::string& inputOrCondBeginBlockStr,
    StateTableElement* pStateTableElement,
    SubElementState* pStateSubElement,
    size_t& i,
    size_t& tabCount)
  {
    auto pInputElement = pStateSubElement->getInputElement();
    auto pInputOrCond = pStateSubElement->getInputOrCondition();
    //Input or condition if-block
    {
      ScopedBlock inputOrCondBlock(line, tabCount, inputOrCondBeginBlockStr);
      generateNextStateAndOutput(inputOrCondBlock, pStateSubElement);
    }
    //Input or condition else-if block(s)
    for(size_t j = (i + 1); j < pStateTableElement->getChildren()->children.size(); j++)
    {
      auto pElseIfStateChild = reinterpret_cast<SubElementState*>(pStateTableElement->getChildren()->getChild(j).get());
      std::string beginBlockString;
      if(std::string::npos != pElseIfStateChild->getState().find("else_if"))
      {
        if(pInputOrCond && pInputOrCond->isType("GrammarElement"))
        {
          beginBlockString.assign("else if(" + pElseIfStateChild->getInputOrCondition()->getNodeAccessor() + "unmarshal(" + pInputElement->getNodeAccessor() + "m_data))");
        }
        else if(pElseIfStateChild->getInputOrCondition())
        {
          std::stringstream elseCond;
          elseCond << "else if(";
          if(pElseIfStateChild->getInputOrCondition()->isType("SubElementActionLine"))
            generateSubElementActionLine(elseCond, pElseIfStateChild->getInputOrCondition().get());
          elseCond << ")";
          beginBlockString.assign(elseCond.str());
        }
      }
      else if(std::string::npos != pElseIfStateChild->getState().find("else"))
        beginBlockString.assign("else");
      else
        break;
      ScopedBlock inputOrCondElseBlock(line, tabCount, beginBlockString);
      generateNextStateAndOutput(inputOrCondElseBlock, pElseIfStateChild);
      i++;
    }
  }

  void clearDataBuffers(ScopedBlock& block, StateTableElement* pElem)
  {
    std::set<std::string> ioElements;
    for(std::shared_ptr<SubElement> pSe : pElem->getChildren()->children)
    {
      auto pStateSubElement = reinterpret_cast<SubElementState*>(pSe.get());
      auto pInputElement = pStateSubElement->getInputElement();
      auto pOutputElement = pStateSubElement->getOutputElement();
      if(pInputElement && ioElements.end() == ioElements.find(pInputElement->getName()))
      {
        if(ioElements.empty()) block << "// Clear IO data buffers in first state\n";
        ioElements.insert(pInputElement->getName());
        block << pInputElement->getNodeAccessor() << "m_data.clear();\n";
      }
      if(pOutputElement && ioElements.end() == ioElements.find(pOutputElement->getName()))
      {
        if(ioElements.empty()) block << "// Clear IO data buffers in first state\n";
        ioElements.insert(pOutputElement->getName());
        block << pOutputElement->getNodeAccessor() << "m_data.clear();\n";
      }
    }
    if(!ioElements.empty())
      block << "\n";
  }
} // End anonymous namespace

StateTableElement::StateTableElement(const std::shared_ptr<Token>& pTok)
: Element(pTok)
{
  setIsUsedInStateTable(true);
}

void StateTableElement::generateCode()
{
  if(!getCppTypeInfo())
    setCppTypeInfo(std::make_shared<CppTypeInfo>());
  std::string spaces = Utils::getTabSpace(1);
  //hpp includes
  getCppTypeInfo()->hppIncludes.push_back("#include <atomic>\n");
  getCppTypeInfo()->hppIncludes.push_back("#include <memory>\n");
  getCppTypeInfo()->hppIncludes.push_back("#include <thread>\n");
  getCppTypeInfo()->hppIncludes.push_back("#include <vector>\n");
  //Declare struct/class in hpp.
  std::string typeName(getCppTypeInfo()->typeNameStr);
  getCppTypeInfo()->hppStatements.push_back("\nclass " + typeName + "\n");
  getCppTypeInfo()->hppStatements.push_back("{\n");
  getCppTypeInfo()->hppStatements.push_back("public:\n");
  getCppTypeInfo()->hppStatements.push_back(spaces + typeName + "();\n");
  getCppTypeInfo()->hppStatements.push_back(spaces + "~" + typeName + "();\n");
  getCppTypeInfo()->hppStatements.push_back(spaces + "void doWork();\n");
  //cpp includes
  getCppTypeInfo()->cppIncludes.push_back("#include <chrono>\n");
  getCppTypeInfo()->cppIncludes.push_back("#include \"" + typeName + ".hpp\"\n");
  std::vector<std::string> states;
  std::string firstState(getChildren()->getChild(0)->getToken()->text);
  std::stringstream line;
  line << "\n";
  size_t tabCount = 0;
  checkRequiresWsaSession();
  //Generate constructor
  {
    auto initListSpaces = Utils::getTabSpace(1);
    ScopedBlock ctor(line, tabCount, typeName + "::" + typeName + "()\n"
      ": " + "m_state(STATES_" + firstState + "),\n"
      + initListSpaces + "m_isRunning(true),\n"
      + initListSpaces + "m_pWorkerThread()\n", 2);
    ctor << "m_pWorkerThread = std::make_shared<std::thread>(&" << typeName << "::doWork, this);\n";
  }
  //Generate destructor
  {
    ScopedBlock dtor(line, tabCount, typeName + "::~" + typeName + "()", 2);
    dtor << "m_pWorkerThread->join();\n";
  }
  //Generate worker thread's doWork function
  {
    ScopedBlock doWorkFunc(line, tabCount, "void " + typeName + "::doWork()", 2);
    if(checkRequiresWsaSession())
      doWorkFunc << "WSASession session; //For socket\n";
    doWorkFunc << "m_isRunning = true;\n";
    doWorkFunc << "// Main state table thread loop\n";

    bool isFirst = true;
    bool isInputOrCondElseIf = false;
    size_t i = 0;
    //While running loop block
    {
      ScopedBlock whileRunningBlock(line, tabCount, "while(m_isRunning)");
      while(i < getChildren()->children.size())
      {
        auto pStateSubElement = reinterpret_cast<SubElementState*>(getChildren()->getChild(i).get());
        auto pInputElement = pStateSubElement->getInputElement();
        auto pInputOrCond = pStateSubElement->getInputOrCondition();
        auto pOutputElement = pStateSubElement->getOutputElement();
        
        states.push_back(pStateSubElement->getState()); //Remember states for hpp enumeration declaration.
        //State block
        {
          std::string enumStateName("STATES_" + pStateSubElement->getState());
          ScopedBlock stateBlock(line, tabCount, "if(" + enumStateName + " == m_state)");
          //IO data buffers cleanup code
          if(isFirst)
          {
            isFirst = false;
            clearDataBuffers(stateBlock, this);
          }
          //Input method block
          if(pInputElement && pInputOrCond && pInputOrCond->isType("GrammarElement"))
          {
            ScopedBlock inputMethodBlock(line, tabCount, getInputReadFuncCall(pStateSubElement));
            std::string ifCondStr("if(" + pInputOrCond->getNodeAccessor() + "unmarshal(" + pInputElement->getNodeAccessor() + "m_data))");
            generateInputOrCondElseIfBlocks(line, ifCondStr, this, pStateSubElement, i, tabCount);
            // Adjust IO buffer if not moving to next state.
            inputMethodBlock << pInputElement->getNodeAccessor() << "adjust(" + enumStateName + " == m_state);\n";
          }
          else if(pInputOrCond)
          {
            std::stringstream ifCond;
            ifCond << "if(";
            if(pInputOrCond->isType("SubElementActionLine"))
              generateSubElementActionLine(ifCond, pInputOrCond.get());
            ifCond << ")";
            generateInputOrCondElseIfBlocks(line, ifCond.str(), this, pStateSubElement, i, tabCount);
          }          
          else if(!pInputOrCond)
          {
            //No input condition
            generateNextStateAndOutput(stateBlock, pStateSubElement);
          }
        } // end state block
        i++;
      }
      whileRunningBlock << "//Yield CPU\n";
      whileRunningBlock << "std::chrono::milliseconds ms(0);\n";
      whileRunningBlock << "std::this_thread::sleep_for(ms);\n";
    } //End of generated While(m_isRunning)
  }//End of generated doWork function
  getCppTypeInfo()->cppStatements.push_back(line.str());
  line.clear();

  getCppTypeInfo()->hppStatements.push_back("private:\n");
  //Generate states enumeration
  if(!states.empty())
  {
    getCppTypeInfo()->hppStatements.push_back(spaces + "enum States\n");
    getCppTypeInfo()->hppStatements.push_back(spaces + "{\n");
    getCppTypeInfo()->hppStatements.push_back(spaces + spaces + "STATES_NONE,");
    std::for_each(states.begin(), states.end(),
      [&](std::string state)
    { getCppTypeInfo()->hppStatements.push_back(spaces + spaces + "STATES_" + state + ",\n"); });
    getCppTypeInfo()->hppStatements.push_back(spaces + "} m_state;\n");
  }
  getCppTypeInfo()->hppStatements.push_back(spaces + "std::atomic<bool> m_isRunning;\n");
  getCppTypeInfo()->hppStatements.push_back(spaces + "std::shared_ptr<std::thread> m_pWorkerThread;");
  //End struct/class in hpp.
  getCppTypeInfo()->hppStatements.push_back("}; //end class " + typeName);

  //Append includes in cpp for element classes accessed by this generated class.
  getCppTypeInfo()->appendAccessedElementIncludes();
}

bool StateTableElement::checkRequiresWsaSession()
{
  bool requiresWsaSession = false;
  for(size_t i = 0; !requiresWsaSession && i < getChildren()->children.size(); i++)
  {
    auto pSubElement = getChildren()->getChild(i);
    auto pStateSubElement = reinterpret_cast<SubElementState*>(pSubElement.get());
    if(pStateSubElement->getInputElement())
    {
      requiresWsaSession =
        Types::ELEMENT_TYPE_IO_INPUT_UDP_IP == pStateSubElement->getInputElement()->getElementType() ||
        Types::ELEMENT_TYPE_IO_OUTPUT_UDP_IP == pStateSubElement->getInputElement()->getElementType();
    }
    if(!requiresWsaSession && pStateSubElement->getOutputElement())
    {
      requiresWsaSession =
        Types::ELEMENT_TYPE_IO_OUTPUT_UDP_IP == pStateSubElement->getOutputElement()->getElementType() ||
        Types::ELEMENT_TYPE_IO_OUTPUT_UDP_IP == pStateSubElement->getOutputElement()->getElementType();
    }
  }
  return requiresWsaSession;
}
