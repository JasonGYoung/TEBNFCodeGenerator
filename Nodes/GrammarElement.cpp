
//Primary include:
#include "GrammarElement.hpp"

GrammarElement::GrammarElement(const std::shared_ptr<Token>& pTok)
  : Element(pTok),
  m_initializerListSet(),
  m_pLastTerminalSubElement()
{
}

void GrammarElement::generateCode()
{
  if(!getCppTypeInfo())
    setCppTypeInfo(std::make_shared<CppTypeInfo>());
  std::string spaces = Utils::getTabSpace();
  //Declare struct/class in hpp.
  std::string typeString("class");
  std::string typeName(getCppTypeInfo()->typeNameStr);
  getCppTypeInfo()->hppStatements.push_back("\n" + typeString + " " + typeName);
  getCppTypeInfo()->hppStatements.push_back("{");
  getCppTypeInfo()->hppStatements.push_back("public:");
  //Generate includes in cpp.
  getCppTypeInfo()->cppIncludes.push_back("//Primary include:");
  getCppTypeInfo()->cppIncludes.push_back("#include \"" + typeName + ".hpp\"");
  getCppTypeInfo()->cppIncludes.push_back("#include \"Conversion.hpp\"");
  getCppTypeInfo()->cppIncludes.push_back("//System includes:");
#if defined(_WIN32) || defined(_WIN64)
  getCppTypeInfo()->cppIncludes.push_back("#include <winsock2.h>   //hton, ntoh, sockets");
#elif __unix
  getCppTypeInfo()->cppIncludes.push_back("#include <netinet/in.h> //hton, ntoh");
  getCppTypeInfo()->cppIncludes.push_back("#include <sys/socket.h> //sockets");
#endif
  //Generate constructor and initializer list in cpp.
  getCppTypeInfo()->cppStatements.push_back("\n" + typeName + "::" + typeName + "()");
  getCppTypeInfo()->cppStatements.push_back(":");
  //Generate member variable declarations and initializer lists.
  std::for_each(getChildren()->children.begin(), getChildren()->children.end(),
    [&](std::shared_ptr<SubElement> pChild)
  { generateDeclCode(pChild); });
  if(!getChildren()->children.empty())
    getCppTypeInfo()->cppStatements.back().append(",\n");
  getCppTypeInfo()->cppStatements.push_back(spaces + "m_data(),");
  getCppTypeInfo()->cppStatements.push_back(spaces + "m_offset(0)");
  //Generate body of constructor in cpp.
  getCppTypeInfo()->cppStatements.push_back("{\n");
  getCppTypeInfo()->cppStatements.push_back("}\n\n");
  //Generate remainder of code after declarations.
  std::for_each(getChildren()->children.begin(), getChildren()->children.end(),
    [&](std::shared_ptr<SubElement> pChild)
  { generateCode(pChild); });
  //Generate includes in hpp.
  getCppTypeInfo()->hppIncludes.push_back("//System includes:");
  getCppTypeInfo()->hppIncludes.push_back("#include <bitset>       //std::bitset");
  getCppTypeInfo()->hppIncludes.push_back("#include <cstdint>      //integral type sizes");
  getCppTypeInfo()->hppIncludes.push_back("#include <memory>       //std::shared_ptr, std::make_shared");
  getCppTypeInfo()->hppIncludes.push_back("#include <mutex>        //std::mutex");
  getCppTypeInfo()->hppIncludes.push_back("#include <string>       //std::string");
  getCppTypeInfo()->hppIncludes.push_back("#include <vector>       //std::vector");
  //Declare constructor in hpp.
  getCppTypeInfo()->hppStatements.push_back(
    Utils::getTabSpace() + getCppTypeInfo()->typeNameStr + "();");
  //Generate marshal/unmarshal function for this element.
  generateMarshalUnmarshalFunction();
  //Generate the actions function called whenever data is unmarshaled.
  generateActionsFunction();
  //Generate getResolvedSize().
  auto pSizeTok = getGrammarSize();
  if(pSizeTok)
  {
    getCppTypeInfo()->hppStatements.push_back(
      Utils::getTabSpace() + "size_t getResolvedSize() { return " + pSizeTok->text + "; }");
  }
  else
  {
    size_t sizeInBytes = 0;
    for(std::shared_ptr<SubElement> pChildSe : getChildren()->children)
    {
      if(pChildSe->getToken()->isSymbolGrammar())
        sizeInBytes += pChildSe->getCppTypeInfo()->resolvedSizeBytes;
    }
    std::stringstream bytesSize;
    bytesSize << sizeInBytes;
    getCppTypeInfo()->hppStatements.push_back(
      Utils::getTabSpace() + "size_t getResolvedSize() { return " + bytesSize.str() + "; }");
  }
  //Static get()
  getCppTypeInfo()->hppStatements.push_back(
    spaces + "static std::shared_ptr<" + getCppTypeInfo()->typeNameStr + "> get()\n"
    + spaces + "{\n"
    + spaces + spaces + "std::lock_guard<std::mutex> lock(m_mutex);\n"
    + spaces + spaces + "static auto pInstance = std::make_shared<" + getCppTypeInfo()->typeNameStr + ">();\n"
    + spaces + spaces + "return pInstance;\n"
    + spaces + "}\n");
  //Utility functions.
  getCppTypeInfo()->hppStatements.push_back("private:\n");
  getCppTypeInfo()->hppStatements.push_back(spaces + "std::vector<uint8_t> m_data;");
  getCppTypeInfo()->hppStatements.push_back(spaces + "size_t m_offset;");
  getCppTypeInfo()->hppStatements.push_back(spaces + "std::mutex m_mutex;");
  //End class in hpp.
  getCppTypeInfo()->hppStatements.push_back("}; //end " + typeString + " " + typeName);
  //Append includes in cpp for element classes accessed by this generated class.
  getCppTypeInfo()->appendAccessedElementIncludes();
}

void GrammarElement::generateMarshalUnmarshalFunction()
{
  auto grammarIsMatchSubElements = getGrammarUnmarshalSubElements();
  if(!getContainingElement()->getToken()->isElementGrammar() || grammarIsMatchSubElements.empty())
    return;
  std::string typeName(getCppTypeInfo()->typeNameStr);
  std::string spaces = Utils::getTabSpace();
  std::string offsetName("m_offset");
  std::string isMatchMarshalFunc(
    "void " + typeName + "::marshal(std::vector<uint8_t>& rData)\n" +
    "{\n" +
    spaces + "if(!rData.empty()) rData.clear();\n" + spaces);
  std::string isMatchUnmarshalFunc(
    "bool " + typeName + "::unmarshal(std::vector<uint8_t>& data)\n" +
    "{\n" +
    spaces + "get().reset(); // Reset to ensure a correct match to data\n" +
    spaces + "m_data.resize(data.size());\n" +
    spaces + "memcpy(&m_data[0], &data[0], data.size());\n" +
    spaces + offsetName + " = 0;\n" +
    spaces + "auto success = ");
  size_t calls = 0;
  auto it = grammarIsMatchSubElements.begin();
  while(it != grammarIsMatchSubElements.end())
  {
    auto pCurSe = *it;
    if(!pCurSe->getToken()->isStaticVariable())
    {
      if(calls > 0)
      {
        isMatchMarshalFunc += "\n" + spaces;
        isMatchUnmarshalFunc += " &&\n" + Utils::getTabSpace(1, 9);
      }
      isMatchMarshalFunc += pCurSe->getCppTypeInfo()->getMarshalCall("", "rData") + ";";
      isMatchUnmarshalFunc += pCurSe->getCppTypeInfo()->getUnmarshalCall("", offsetName);
      it = grammarIsMatchSubElements.erase(it);
      ++calls;
    }
    else
      ++it;
  }
  isMatchMarshalFunc += "}\n\n";
  isMatchUnmarshalFunc += ";\n";
  isMatchUnmarshalFunc += spaces + "if(success) actions();\n";
  isMatchUnmarshalFunc += spaces + "return success;\n}\n\n";
  getCppTypeInfo()->cppStatements.push_back(isMatchMarshalFunc);
  getCppTypeInfo()->cppStatements.push_back(isMatchUnmarshalFunc);
  getCppTypeInfo()->hppStatements.push_back(spaces + "void " + "marshal(std::vector<uint8_t>& rData);\n");
  getCppTypeInfo()->hppStatements.push_back(spaces + "bool " + "unmarshal(std::vector<uint8_t>& data);\n");
}

void GrammarElement::generateDeclCode(std::shared_ptr<SubElement> pThisSubElement)
{
  std::shared_ptr<CppTypeInfo> pLastCppInfo;
  bool allChildrenSameType = pThisSubElement->getChildren()->isAllChildrenSame(pLastCppInfo);
  if(pThisSubElement->hasChildren())
  {
    std::string varName(pThisSubElement->getCppTypeInfo() ? pThisSubElement->getCppTypeInfo()->typeNameStr : std::string());
    std::string varType(pThisSubElement->getCppTypeInfo() ? pThisSubElement->getCppTypeInfo()->typeStr : std::string());
    for(size_t i = 0; i < pThisSubElement->getChildren()->children.size(); i++)
    {
      auto pCurChild = pThisSubElement->getChildren()->getChild(i);
      if(varType.empty())
      {
        if(allChildrenSameType && pLastCppInfo && !pLastCppInfo->typeStr.empty())
          varType = pLastCppInfo->typeStr;
        else
          varType = "int8_t";
      }
      else if(!allChildrenSameType && pThisSubElement->getCppTypeInfo()->isVector())
      {
        varType = "int8_t";
      }
      size_t containerSize = 0;
      auto pCppInfo = pCurChild->getCppTypeInfo();
      if(pCppInfo && pCppInfo->pTypeRange)
        containerSize = pCppInfo->pTypeRange->getSize();
      else if(pThisSubElement->getCppTypeInfo() && pThisSubElement->getCppTypeInfo()->pTypeRange)
        containerSize = pThisSubElement->getCppTypeInfo()->pTypeRange->getSize();

      std::stringstream cppLine;
      if(pThisSubElement->getCppTypeInfo() && pThisSubElement->getCppTypeInfo()->isVector() && containerSize > 0)
        cppLine << varName << "(" << containerSize << ")";
      else if(pThisSubElement->getCppTypeInfo() && pThisSubElement->getCppTypeInfo()->isString() && containerSize > 0)
      {
        std::string literalStr = pThisSubElement->getChildren()->getLastChild()->getToken()->text;
        cppLine << varName << "(" << literalStr << ")";
      }
      else if(pThisSubElement->getCppTypeInfo() && !pThisSubElement->getCppTypeInfo()->isVector())
        cppLine << varName << "(0)";
      else
        cppLine << varName << "()";
      if(this->getCppTypeInfo()->cppStatements.back() == ":")
      {
        std::string spaces = Utils::getTabSpace(1, Utils::TAB_SIZE - 1);
        this->getCppTypeInfo()->cppStatements.back().append(spaces + cppLine.str());
      }
      else if(this->getCppTypeInfo()->cppStatements.back().find(cppLine.str()) == std::string::npos)
      {
        this->getCppTypeInfo()->cppStatements.back().append(",\n");
        this->getCppTypeInfo()->cppStatements.push_back(Utils::getTabSpace() + cppLine.str());
      }
      std::string spaces = Utils::getTabSpace();
      std::string hppLineStr;
      if(pThisSubElement->getCppTypeInfo()->isBitset())
      {
        std::ostringstream bitsetLine;
        bitsetLine << spaces << "std::bitset<" << containerSize << "> " << varName << ";";
        hppLineStr = bitsetLine.str();
      }
      else if(pThisSubElement->getCppTypeInfo()->isString())
      {
        hppLineStr = spaces + varType + " " + varName + ";";
      }
      else
      {
        hppLineStr = containerSize > 0 || pThisSubElement->getCppTypeInfo()->isVector() ?
          spaces + "std::vector<" + varType + "> " + varName + ";" :
          spaces + varType + " " + varName + ";";
      }
      if(this->getCppTypeInfo()->hppStatements.empty() ||
        this->getCppTypeInfo()->hppStatements.back() != hppLineStr)
        this->getCppTypeInfo()->hppStatements.push_back(hppLineStr);
    }
  }
  else if(pThisSubElement->getToken()->isStaticVariable())
  {
    std::string hppLineStr(Utils::getTabSpace() + "StaticVariable " + Utils::getCppVarName(pThisSubElement->getToken(), false) + ";\n");
    if(this->getCppTypeInfo()->hppStatements.empty() ||
      this->getCppTypeInfo()->hppStatements.back() != hppLineStr)
      this->getCppTypeInfo()->hppStatements.push_back(hppLineStr);

    if(this->getCppTypeInfo()->hppIncludes.end() ==
      std::find(this->getCppTypeInfo()->hppIncludes.begin(),
      this->getCppTypeInfo()->hppIncludes.end(), "#include \"StaticVariable.hpp\"\n"))
    {
      this->getCppTypeInfo()->hppIncludes.push_back("#include \"StaticVariable.hpp\"\n");
    }
  }

  if(pThisSubElement->getToken()->isOperatorTermination() &&
    pThisSubElement->getCppTypeInfo() &&
    !pThisSubElement->getCppTypeInfo()->cppStatements.empty())
  {
    this->getCppTypeInfo()->cppStatements.push_back(";");
  }
}

void GrammarElement::generateCode(std::shared_ptr<SubElement> pThisSubElement)
{
  std::string implMarshal, declMarshal, implUnmarshal, declUnmarshal, implMatch, declMatch;
  auto pChild = pThisSubElement->getSharedFromThis();
  if(getMarshalFunction(pThisSubElement->getCppTypeInfo(), pChild, implMarshal, declMarshal))
  {
    this->getCppTypeInfo()->hppStatements.push_back(declMarshal);
    this->getCppTypeInfo()->cppStatements.push_back(implMarshal);
  }
  if(getUnmarshalFunction(pThisSubElement->getCppTypeInfo(), pChild, implUnmarshal, declUnmarshal))
  {
    this->getCppTypeInfo()->hppStatements.push_back(declUnmarshal);
    this->getCppTypeInfo()->cppStatements.push_back(implUnmarshal);
  }
}

void GrammarElement::generateActionsFunction()
{
  std::string lines;
  std::for_each(getChildren()->children.begin(), getChildren()->children.end(),
    [&](std::shared_ptr<SubElement> pChild)
  {
    if(pChild->getToken()->isStaticVariable())
    {
      std::string line;
      auto pSeActionLine = std::dynamic_pointer_cast<SubElementActionLine>(pChild);
      auto rpnTokens = pSeActionLine->getRpnTokens();
      for(size_t i = 0; i < rpnTokens.size(); i++)
      {
        auto pTok = rpnTokens.at(i);
        if(!line.empty()) line.append(" ");
        if(pTok->isSymbolTyped())
        {
          std::shared_ptr<CppTypeInfo> pCppInfo;
          if(TypeUtils::getCppTypeInfo(pTok, pCppInfo))
          {
            auto findIt = std::find_if(
              getCppTypeInfo()->cppStatements.begin(), getCppTypeInfo()->cppStatements.end(),
              [&](const std::string& statement)->bool{ return std::string::npos != statement.find("m_offset(0)"); });
            auto pLastTok = Utils::getItemAt(rpnTokens, i - 1);
            auto pLastLastTok = Utils::getItemAt(rpnTokens, i - 2);
            //Insert it into the initializer list only if it is a member of this element.
            bool isStaticVarTypeAssignment =
              pLastTok && pLastTok->isOperatorAssignment(true) &&
              pLastLastTok && pLastLastTok->isStaticVariable() &&
              getChildren()->findChild(pLastLastTok->text);
            if(findIt != getCppTypeInfo()->cppStatements.end() && isStaticVarTypeAssignment)
            {
              std::string initSv(Utils::getTabSpace() + Utils::getCppVarName(pLastLastTok, false) + "(static_cast<" + pCppInfo->typeStr + ">(0)),\n");
              getCppTypeInfo()->cppStatements.insert(findIt, initSv);
              line.clear();
            }
          }
        }
        else
          line.append(Utils::getCppVarName(pTok));
      }
      if(!line.empty())
        lines.append(Utils::getTabSpace(2) + line + ";\n");
    }
  });
  if(lines.empty())
    lines = Utils::getTabSpace(2) + "/* No actions to execute */\n";
  this->getCppTypeInfo()->hppStatements.push_back(Utils::getTabSpace() + "void actions();\n");
  std::string spaces = Utils::getTabSpace();
  this->getCppTypeInfo()->cppStatements.push_back(
    "void " + this->getCppTypeInfo()->getTypeNameStr() + "::actions()\n"
    "{\n"
    + spaces + "/* Execute actions within try-catch to minimize crashes */\n"
    + spaces + "try\n"
    + spaces + "{\n"
    + lines
    + spaces + "}\n"
    + spaces + "catch(const std::exception& ex)\n"
    + spaces + "{\n"
    + spaces + spaces + "std::cerr << \"An error occurred in actions(): \" << std::string(ex.what()) << std::endl;\n"
    + spaces + "}\n"
    + spaces + "catch(...)\n"
    + spaces + "{\n"
    + spaces + spaces + "std::cerr << \"An unknown error occurred in actions()\" << std::endl;\n"
    + spaces + "}\n"
    "}\n\n");
}

namespace
{
  size_t marshalLitIdx = 0;

  void getMarshalFunctionHelper(
    CppTypeInfo* pCppInfo,
    const std::string& spaces,
    std::string& rImpl)
  {
    std::stringstream ss;
    std::string varName(Utils::getCppVarName(*pCppInfo));
    if("std::bitset" == pCppInfo->typeStr)
    {
    }
    else
    {
      if(pCppInfo->isNumStr)
        ss << spaces << "Conversion::marshalNumString(rData, " << varName << ", " << pCppInfo->resolvedSizeBytes << ", true);\n";
      else
      {
        if(pCppInfo->isLiteral())
        {
          auto pParent = pCppInfo->pCppTypeNode->getParent();
          if(pParent && pParent->getToken()->isSymbolGrammar() && pParent->getCppTypeInfo())
          {
            varName = Utils::getCppVarName(*pParent->getCppTypeInfo());
            if(pParent->getCppTypeInfo()->isVector())
            {
              std::stringstream arrayIdx;
              arrayIdx << "[" << marshalLitIdx << "]";
              varName.append(arrayIdx.str());
            }
          }
        }
        ss << spaces << "Conversion::marshal(rData, " << varName << ", true);\n";
      }
    }
    rImpl += ss.str();
    marshalLitIdx += pCppInfo->resolvedSizeBytes;
  }
}
bool GrammarElement::getMarshalFunction(
  std::shared_ptr<CppTypeInfo> pMarshalCppInfo,
  std::shared_ptr<SubElement> pSubElement,
  std::string& rImpl,
  std::string& rDecl,
  bool isBaseCase)
{
  if(!this || !pSubElement)
    return false;

  std::string spaces = Utils::getTabSpace();
  if(isBaseCase)
  {
    marshalLitIdx = 0;
    std::ostringstream impl, decl;
    if(pSubElement->getToken()->isStaticVariable())
      return false;
    std::string typeName = pSubElement->getContainingElement()->getCppTypeInfo()->typeNameStr;
    impl << "void " << typeName << "::marshal_" << pMarshalCppInfo->typeNameStr << "(std::vector<uint8_t>& rData)\n"
      << "{\n";
    rImpl += impl.str();
    bool success = getMarshalFunction(pMarshalCppInfo, pSubElement, rImpl, rDecl, false);
    //  if(pSubElement->getCppTypeInfo()->pTypeSizeCastSubElement)
    //  {
    //    std::string sizeStr(pSubElement->getCppTypeInfo()->getSizeStr());
    //    rImpl += spaces + "if(rData.size() != " + sizeStr + ") rData.resize(" + sizeStr + "); /*Type size cast*/\n";
    //  }
    rImpl += "}\n\n";
    //Function declaration.
    spaces = Utils::getTabSpace();
    rDecl = spaces + "/** Marshal " + pMarshalCppInfo->typeNameStr + " from class to binary. */\n" +
      spaces + "void marshal_" + pMarshalCppInfo->typeNameStr + "(std::vector<uint8_t>& rData);\n";
    return success;
  }
  //Recurse through subelements.
  static std::string elementName;
  static std::string lastOffsetVar;
  auto pElement = pSubElement->getContainingElement();
  if(elementName != pElement->getName())
  {
    lastOffsetVar.clear();
    elementName = pElement->getName();
  }
  if(pSubElement->hasChildren())
  {
    for(size_t i = 0; i < pSubElement->getChildren()->children.size(); i++)
    {
      std::shared_ptr<SubElement> pSe = pSubElement->getChildren()->children.at(i);
      if((pSe->getRelationToParent() && Token::OPERATOR_ASSIGN == pSe->getRelationToParent().get()) ||
         (pSe->getRelationToSibling() && Token::OPERATOR_CONCAT == pSe->getRelationToSibling().get()) ||
         (pSe->getRelationToSibling() && Token::OPERATOR_OR == pSe->getRelationToSibling().get()))
      {
        if(pSe->getToken()->isSymbolGrammar())
        {
          if(pSe->getToken()->pAccessedElementToken)
            getMarshalFunctionHelper(pSe->getCppTypeInfo().get(), spaces, rImpl);
          else
          {
            static std::string marshalTypeName = Utils::getCppVarName(pSubElement->getContainingElement()->getName());
            auto pSeInfo = pSe->getCppTypeInfo(true);
            rImpl += spaces + "" + pSeInfo->getMarshalCall(marshalTypeName + "::") + ";\n";
            marshalLitIdx += pSubElement->getCppTypeInfo()->resolvedSizeBytes;
          }
        }
        else if(pSe->getToken()->isStaticVariable())
        {

        }
        else if(pSe->getToken()->isLiteral())
        {
          bool isOrRelationship = false;
          if((i + 1) < pSubElement->getChildren()->children.size())
          {
            std::shared_ptr<SubElement> pNextSe = pSubElement->getChildren()->children.at(i + 1);
            isOrRelationship = Token::OPERATOR_OR == pNextSe->getRelationToSibling().get();
          }
          if(!isOrRelationship)
            getMarshalFunctionHelper(pSe->getCppTypeInfo().get(), spaces, rImpl);
        }
        else
          getMarshalFunction(pMarshalCppInfo, pSe, rImpl, rDecl, false);
      }
    }
  }
  else
    getMarshalFunctionHelper(pMarshalCppInfo.get(), spaces, rImpl);

  return true;
}

namespace
{
  size_t unmarshalLitIdx = 0;
  std::string unmarshalTypeName;
  std::string unmarshalReturnVal;

  void getUnmarshalFunctionHelper(CppTypeInfo* pCppInfo,
    const std::string& spaces,
    std::string& rImpl)
  {
    std::stringstream ss;
    std::string varName(Utils::getCppVarName(*pCppInfo));
    if("std::bitset" == pCppInfo->typeStr)
    {
    }
    else
    {
      if(pCppInfo->isLiteral())
      {
        auto pParent = pCppInfo->pCppTypeNode->getParent();
        if(pParent && pParent->getToken()->isSymbolGrammar() && pParent->getCppTypeInfo())
        {
          varName = Utils::getCppVarName(*pParent->getCppTypeInfo());
          if(pParent->getCppTypeInfo()->isVector())
          {
            std::stringstream arrayIdx;
            arrayIdx << "[" << unmarshalLitIdx << "]";
            varName.append(arrayIdx.str());
          }
        }
      }
      if(pCppInfo->pTypeRange &&
        pCppInfo->isVector() &&
        pCppInfo->pTypeRange->pMaxToken &&
        (pCppInfo->pTypeRange->pMaxToken->isSymbolGrammar() ||
        pCppInfo->pTypeRange->pMaxToken->isStaticVariable()))
      {
        //Grammar subelement or static variable used as range value.
        auto pElem = pCppInfo->pCppTypeNode->getContainingElement();
        auto pChild = pElem->getChildren()->findChild(pCppInfo->pTypeRange->pMaxToken->text);
        if(pChild)
          ss << spaces << varName << ".resize(" << Utils::getCppVarName(pChild->getCppTypeInfo()) << ");\n";
      }
      if(pCppInfo->isNumStr)
        ss << spaces << "if(!Conversion::unmarshalNumString(m_data, " << varName << ", " << pCppInfo->resolvedSizeBytes << ", " << DATA_OFFSET;
      else
      {
        if(pCppInfo->isLiteral() && !pCppInfo->isString())
          ss << spaces << "if(!Conversion::unmarshal(m_data, " << varName << "[" << unmarshalLitIdx << "], " << DATA_OFFSET;
        else
          ss << spaces << "if(!Conversion::unmarshal(m_data, " << varName << ", " << DATA_OFFSET;
        if(pCppInfo->isLiteral() && pCppInfo->isString() && pCppInfo->pCppTypeNode && pCppInfo->pCppTypeNode->hasChildren())
        {
          auto pStrValChild = pCppInfo->pCppTypeNode->getChildren()->getChild(0);
          if(pStrValChild->getToken()->isLiteralString())
            ss << ", " << pStrValChild->getToken()->text;
        }
        if(pCppInfo->diffBytes > 0)
          ss << ", " << pCppInfo->diffBytes << "/*size from grammar = 8 + " << pCppInfo->diffBytes << "*/";
      }
      ss << ")) return false;\n";
    }
    rImpl += ss.str();
  }
}
bool GrammarElement::getUnmarshalFunction(
  std::shared_ptr<CppTypeInfo> pUnmarshalCppInfo,
  std::shared_ptr<SubElement> pSubElement,
  std::string& rImpl,
  std::string& rDecl,
  bool isBaseCase)
{
  if(!this || !pSubElement)
    return false;

  std::string spaces(Utils::getTabSpace());
  if(isBaseCase)
  {
    if(pSubElement->getToken()->isStaticVariable())
      return false;
    unmarshalLitIdx = 0;
    unmarshalTypeName.clear();
    unmarshalReturnVal.clear();
    std::string typeName = pSubElement->getContainingElement()->getCppTypeInfo()->typeNameStr;
    rImpl += "bool " + typeName + "::unmarshal_" + pUnmarshalCppInfo->typeNameStr + "(size_t& " + DATA_OFFSET + ")\n{\n";

    /* if(pUnmarshalCppInfo->pTypeSizeCastSubElement)
    {
    rImpl += spaces + "skip2.resize(" + pUnmarshalCppInfo->pTypeSizeCastSubElement->getCppTypeInfo()->getSizeStr() + " - rDataOffset);\n";
    }*/
    auto pLastTermSubElem = getLastTerminalSubElement();
    if(pLastTermSubElem &&
      pLastTermSubElem->getToken()->text == pSubElement->getToken()->text &&
      getGrammarSize())
    {
      std::string varName(Utils::getCppVarName(pUnmarshalCppInfo));
      rImpl += spaces + varName + ".resize(" + getGrammarSize()->text + " - rDataOffset);\n";
    }

    std::string funcImpl;
    bool success = getUnmarshalFunction(pUnmarshalCppInfo, pSubElement, funcImpl, rDecl, false);
    const std::string offsetStr("size_t " + UNMARSHAL_OFFSET + " = 0;");
    if(std::string::npos != funcImpl.find(UNMARSHAL_OFFSET))
      rImpl += spaces + offsetStr + "\n";
    rImpl += funcImpl;
    spaces = Utils::getTabSpace();
    rImpl += spaces + "return " + (unmarshalReturnVal.empty() ? "true" : unmarshalReturnVal) + ";\n";
    rImpl += "}\n\n";
    //Function declaration.
    rDecl = spaces + "/** Unmarshal " + pUnmarshalCppInfo->typeNameStr + " from binary to class. */\n" +
      spaces + "bool unmarshal_" + pUnmarshalCppInfo->typeNameStr + "(size_t& " + DATA_OFFSET + ");\n";
    return success;
  }

  std::ostringstream impl, decl;
  if(pSubElement->hasChildren())
  {
    for(size_t i = 0; i < pSubElement->getChildren()->children.size(); i++)
    {
      std::shared_ptr<SubElement> pSe = pSubElement->getChildren()->children.at(i);
      if((pSe->getRelationToParent() && Token::OPERATOR_ASSIGN == pSe->getRelationToParent().get()) ||
         (pSe->getRelationToSibling() && Token::OPERATOR_CONCAT == pSe->getRelationToSibling().get()) ||
         (pSe->getRelationToSibling() && Token::OPERATOR_OR == pSe->getRelationToSibling().get()))
      {
        if(pSe->getToken()->isSymbolGrammar())
        {
          if(pSe->getToken()->pAccessedElementToken)
            getUnmarshalFunctionHelper(pSe->getCppTypeInfo().get(), spaces, rImpl);
          else
          {
            static std::string unmarshalTypeName = Utils::getCppVarName(pSubElement->getContainingElement()->getName());
            auto pSeInfo = pSe->getCppTypeInfo(true);
            rImpl += spaces + "if(!" + pSeInfo->getUnmarshalCall(unmarshalTypeName + "::") + ") return false;\n";
            //marshalLitIdx += pSubElement->getCppTypeInfo()->resolvedSizeBytes;
          }
        }
        else if(pSe->getToken()->isLiteral())
        {
          bool isOrRelationship = false;
          if((i + 1) < pSubElement->getChildren()->children.size())
          {
            std::shared_ptr<SubElement> pNextSe = pSubElement->getChildren()->children.at(i + 1);
            isOrRelationship = Token::OPERATOR_OR == pNextSe->getRelationToSibling().get();
          }
          std::string retValOp("&&");
          if(pSe->getRelationToSibling() && Token::OPERATOR_OR == pSe->getRelationToSibling().get())
            retValOp = "||";
          std::string litText("static_cast<" + pSe->getCppTypeInfo()->typeStr + ">(" + pSe->getToken()->text + ")");
          if(!unmarshalReturnVal.empty())
            unmarshalReturnVal += " " + retValOp + " \n" + spaces + Utils::getTabSpace(1, 7);
          size_t litSize = pSe->getCppTypeInfo()->resolvedSizeBytes;
          if(pUnmarshalCppInfo->isString())
            unmarshalReturnVal += pSe->getToken()->text + " == " + Utils::getCppVarName(pUnmarshalCppInfo);
          else
          {
            std::stringstream ss;
            ss << "Conversion::compare(&" << Utils::getCppVarName(pUnmarshalCppInfo) << "[" << unmarshalLitIdx << "], " << litText << ")";
            unmarshalReturnVal += ss.str();
          }
          if(!isOrRelationship)
          {
            getUnmarshalFunctionHelper(pSe->getParent()->getCppTypeInfo().get(), spaces, rImpl);
            unmarshalLitIdx += litSize;
          }
        }
        else if(pSe->getToken()->isSymbolTyped())
          getUnmarshalFunctionHelper(pSe->getParent()->getCppTypeInfo().get(), spaces, rImpl);
        else
          getUnmarshalFunction(pUnmarshalCppInfo, pSe, rImpl, rDecl, false);
      }
    }
  }
  else
    getUnmarshalFunctionHelper(pUnmarshalCppInfo->pCppTypeNode->getCppTypeInfo().get(), spaces, rImpl);

  if(pSubElement->hasChildren() && pUnmarshalCppInfo->pTypeSizeCastSubElement)
  {

  }

  return true;
}
