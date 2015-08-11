

// Primary include:
#include "ActionsElement.hpp"

ActionsElement::ActionsElement(const std::shared_ptr<Token>& pTok)
: Element(pTok),
  m_params(),
  m_args()
{
}

void ActionsElement::generateCode()
{
  if(!hasChildren()) return;
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

  std::string lines;
  for(std::shared_ptr<SubElement> pSe : getChildren()->children)
  {
    if("SubElementActionLine" != pSe->getTypeName()) continue;
    auto pSeActionLine = std::dynamic_pointer_cast<SubElementActionLine>(pSe);
    std::string line;
    for(std::shared_ptr<Token> pTok : pSeActionLine->getRpnTokens())
    {
      if(pTok->isStaticVariable())
      {
        if(getCppTypeInfo()->hppIncludes.end() ==
           std::find(getCppTypeInfo()->hppIncludes.begin(),
             getCppTypeInfo()->hppIncludes.end(), "#include \"StaticVariable.hpp\"\n"))
        {
          getCppTypeInfo()->hppIncludes.push_back("#include \"StaticVariable.hpp\"\n");
        }
      }
      if(!line.empty())
        line.append(" ");
      line.append(Utils::getCppVarName(pTok));
    }
    lines.append(spaces + spaces + line + ";\n");
  }
   
  std::string params(getParamString());

  getCppTypeInfo()->hppStatements.push_back(Utils::getTabSpace() + "static void doActions(" + params + ");\n");
  getCppTypeInfo()->cppStatements.push_back(
    "void " + getCppTypeInfo()->getTypeNameStr() + "::doActions(" + params + ")\n"
    "{\n"
    + spaces + "/* Execute actions within try-catch to minimize crashes */\n"
    + spaces + "try\n"
    + spaces + "{\n"
    + lines
    + spaces + "}\n"
    + spaces + "catch(const std::exception& ex)\n"
    + spaces + "{\n"
    + spaces + spaces + "std::cerr << \"An error occurred in doActions(): \" << std::string(ex.what()) << std::endl;\n"
    + spaces + "}\n"
    + spaces + "catch(...)\n"
    + spaces + "{\n"
    + spaces + spaces + "std::cerr << \"An unknown error occurred in doActions()\" << std::endl;\n"
    + spaces + "}\n"
    "}\n\n");

  //End class in hpp.
  getCppTypeInfo()->hppStatements.push_back("}; //end " + typeString + " " + typeName);
  //Append includes in cpp for element classes accessed by this generated class.
  getCppTypeInfo()->appendAccessedElementIncludes();
  getCppTypeInfo()->cppIncludes.push_back("\n");
}

std::string ActionsElement::getParamString() const
{
  std::string params;
  for(std::shared_ptr<Token> pParam : m_params)
  {
    if(!params.empty()) params.append(", ");
    params.append("const StaticVariable& " + pParam->text);
  }
  return params;
}

std::string ActionsElement::getActionsFunctionCall() const
{
  std::string funcCall(Utils::getCppVarName(getCppTypeInfo()) + "::doActions(");
  std::string paramStr;
  for(std::shared_ptr<Token> pArg : m_args)
  {
    if(!paramStr.empty()) paramStr.append(", ");
    paramStr.append(pArg->text);
  }
  if(!paramStr.empty())
    funcCall.append(paramStr);
  funcCall.append(")");
  return funcCall;
}
