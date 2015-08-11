
//Primary include:
#include "CppTypeInfo.hpp"

//Local includes:
#include "Element.hpp"
#include "Elements.hpp"
#include "../Token.hpp"
#include "../Utils/Utils.hpp"
//#include <winsock2.h>

#include <fstream>

TypeRange::TypeRange(std::shared_ptr<SubElement> pSubElement,
                     const std::vector<std::shared_ptr<Token> >& tokens)
: minValue(0),
  maxValue(0),
  pMinToken(),
  pMaxToken(),
  vectorType(TYPE_VEC_UNKNOWN)
{
  std::shared_ptr<Token> pLastToken;
  for(size_t i = 0; i < tokens.size(); i++)
  {
    auto pTok = Utils::getItemAt(tokens, i);
    if(pLastToken)
    {
      if(pLastToken && pLastToken->isRangeValue())
      {
        if(Token::TYPE_NONE != pLastToken->actualType &&
            Token::UNKNOWN != pLastToken->actualType)
          pLastToken->type = pLastToken->actualType;
        std::string txt = pLastToken->text;
        auto pSe = pSubElement->findNode(txt);
        if(pTok->isOperatorRangeDelim())
        {
          pMinToken = pLastToken;
          if(pSe)
            pMinToken->pNode = pSe.get();
        }
        else if(pTok->isOperatorRightRangeBracket())
        {
          pMaxToken = pLastToken;
          if(pSe)
            pMaxToken->pNode = pSe.get();
        }
      }
    }
    pLastToken = pTok;
  }
  vectorType = pMinToken && pMaxToken ? TYPE_VEC_MINMAX_BOUNDED :
    !pMinToken && pMaxToken && 3 == tokens.size() ? TYPE_VEC_BOUNDED :
    pMinToken && !pMaxToken ? TYPE_VEC_MAX_UNBOUNDED :
    !pMinToken && pMaxToken ? TYPE_VEC_MIN_UNBOUNDED :
    !pMinToken && !pMaxToken ? TYPE_VEC_UNBOUNDED :
    TYPE_VEC_UNKNOWN;
  minValue = pMinToken ? Utils::stoi(pMinToken->text) : 0;
  maxValue = pMaxToken ? Utils::stoi(pMaxToken->text) : 0;
  if(TYPE_VEC_MINMAX_BOUNDED == vectorType &&
     minValue > maxValue &&
     !pMaxToken->isSymbolGrammar() &&
     !pMaxToken->isStaticVariable())
    Utils::Logger::logErr(pMinToken, "Minimum range value cannot exceed maximum range value");
}

TypeRange::TypeRange(size_t minVal, size_t maxVal)
: minValue(minVal),
  maxValue(maxVal),
  pMinToken(std::make_shared<Token>(Token::LITERAL_DEC, std::to_string(minValue))),
  pMaxToken(std::make_shared<Token>(Token::LITERAL_DEC, std::to_string(maxValue))),
  vectorType((minVal > 0 && maxVal > 0) ? TYPE_VEC_MINMAX_BOUNDED :
  (minVal == 0 && maxVal > 0) ? TYPE_VEC_BOUNDED :
  TYPE_VEC_UNBOUNDED)
{
}

std::string TypeRange::getMaxRangeStr()
{
  if(pMaxToken->pNode && (pMaxToken->isSymbolGrammar() || pMaxToken->isStaticVariable()))
    return pMaxToken->pNode->getCppTypeInfo()->getMember(Utils::none, "");
  else if(pMaxToken->isLiteral() && maxValue > 0)
    return pMaxToken->text;
  return std::string();
}

std::string TypeRange::getMinRangeStr()
{
  if(pMinToken && pMinToken->pNode && (pMinToken->isSymbolGrammar() || pMinToken->isStaticVariable()))
    return pMinToken->pNode->getCppTypeInfo()->getMember(Utils::none, "");
  else if(pMinToken && pMinToken->isLiteral() && minValue > 0)
    return pMinToken->text;
  return std::string();
}

bool CppTypeInfo::isStaticVariable() const
{
  return this && pCppTypeNode && pCppTypeNode->getToken()->isStaticVariable();
}

std::string CppTypeInfo::getSizeStr()
{
  std::stringstream szStrm;
  if(pTypeSizeCastSubElement)
  {
    auto pCastCppTypeInfo = pTypeSizeCastSubElement->getCppTypeInfo();
    if(pCastCppTypeInfo->pTypeRange)
    {
      std::string maxStr(pCastCppTypeInfo->pTypeRange->getMaxRangeStr());
      if(maxStr.empty())
        szStrm << pCastCppTypeInfo->pTypeRange->getMinRangeStr();
      else
        szStrm << maxStr;
    }
    else if(pCastCppTypeInfo->resolvedSizeBytes > 0)
      szStrm << pCastCppTypeInfo->resolvedSizeBytes;
  }
  else
    szStrm << resolvedSizeBytes;
  return szStrm.str();
}

std::string CppTypeInfo::getMarshalCall(const std::string& typeName,
                                        const std::string& dataBuf)
{
  std::string callStr("marshal_" + typeNameStr);
  if(pCppTypeNode && pCppTypeNode->isElement())
    callStr.assign("marshal");
  if(typeName.empty())
    return callStr + "(" + dataBuf + ")";
  return typeName + callStr + "(" + dataBuf + ")";
}

std::string CppTypeInfo::getUnmarshalCall(const std::string& typeName,
                                          const std::string& offsetVarName)
{
  std::string callStr("unmarshal_" + typeNameStr);
  if(pCppTypeNode && pCppTypeNode->isElement())
    callStr.append("unmarshal");
  if(typeName.empty())
    return callStr + "(" + offsetVarName + ")";
  return typeName + callStr + "(" + offsetVarName + ")";
}

std::string CppTypeInfo::getTypeNameStr(bool typeQualify)
{
  if(typeQualify && pCppTypeNode && pCppTypeNode->getToken()->pAccessedElementToken)
    return Utils::getCppVarName(pCppTypeNode->getToken());
  return Utils::getCppVarName(typeNameStr);
}

void CppTypeInfo::appendAccessedElementIncludes()
{
  bool isCommentAdded = false;
  auto accessedElementNames = Utils::getCppVarNameAccessedElementNames(true);
  if(!accessedElementNames.empty())
  {
    std::for_each(accessedElementNames.begin(), accessedElementNames.end(),
      [&](std::string elemName)
    {
      if(elemName != typeNameStr)
      {
        if(!isCommentAdded)
        {
          cppIncludes.push_back("//Accessed element includes and typedefs:\n");
          isCommentAdded = true;
        }
        auto pElem = Elements::findElement("@" + elemName);
        if(pElem && pElem->isAsElement())
        {
          std::string typedefOf(Utils::getCppVarName(pElem->getChildren()->pChildElement->getCppTypeInfo()));
          auto findIt = std::find_if(accessedElementNames.begin(), accessedElementNames.end(),
            [typedefOf](const std::string& name)->bool { return std::string::npos != name.find(typedefOf); });
          if(findIt == accessedElementNames.end())
            cppIncludes.push_back("#include \"" + typedefOf + ".hpp\"\n");
          cppIncludes.push_back("typedef " + typedefOf + " " + elemName + "; //AS\n");
        }
        else
          cppIncludes.push_back("#include \"" + elemName + ".hpp\"\n");
      }
    });
  }
}

namespace
{
  void writeLinesToFile(std::ofstream& fs, std::vector<std::string>& lines)
  {
    std::for_each(lines.begin(), lines.end(),
      [&](const std::string line)
      {
        if(!line.empty())
        {
          fs << line;
          if('\n' != line.back())
            fs << "\n";
        }
      });
  }
}

void CppTypeInfo::writeCodeToDisk(const std::string& dirPath)
{
  std::string errMsg("An error occurred while writing \"" + typeNameStr + "\" to disk");
  try
  {
    if(!hppStatements.empty())
    {
      std::string typeNameStrUppercase = Utils::toUpperCopy(typeNameStr);
      std::ofstream hppFs(dirPath + "/" + typeNameStr + ".hpp", std::ios::out);
      hppFs << Utils::getClassPrefaceComment(typeNameStr + ".hpp")
            << "#ifndef " << typeNameStrUppercase << "_HPP\n"
            << "#define " << typeNameStrUppercase << "_HPP\n\n";
      writeLinesToFile(hppFs, hppIncludes);
      writeLinesToFile(hppFs, hppStatements);
      hppFs << "#endif //" << typeNameStrUppercase << "_HPP\n";
      hppFs.close();
    }
    if(!cppStatements.empty())
    {
      std::string fname(typeNameStr);
      if(isCppMain) fname.append("_Main");
      fname.append(".cpp");
      std::ofstream cppFs(dirPath + "/" + fname, std::ios::out);
      cppFs << Utils::getClassPrefaceComment(fname);
      writeLinesToFile(cppFs, cppIncludes);
      writeLinesToFile(cppFs, cppStatements);
      cppFs.close();
    }
    if(!supportClasses.empty())
    {
      std::for_each(supportClasses.begin(), supportClasses.end(),
        [&](std::shared_ptr<SupportClass> pSupportClass)
        {
          if(!pSupportClass->hppStatements.empty())
          {
            std::ofstream supportClassHpp(dirPath + "/" + pSupportClass->typeName + ".hpp", std::ios::out);
            supportClassHpp << Utils::getClassPrefaceComment(pSupportClass->typeName + ".hpp");
            writeLinesToFile(supportClassHpp, pSupportClass->hppIncludes);
            writeLinesToFile(supportClassHpp, pSupportClass->hppStatements);
            supportClassHpp.close();
          }
          if(!pSupportClass->cppStatements.empty())
          {
            std::ofstream supportClassCpp(dirPath + "/" + pSupportClass->typeName + ".cpp", std::ios::out);
            supportClassCpp << Utils::getClassPrefaceComment(pSupportClass->typeName + ".cpp");
            writeLinesToFile(supportClassCpp, pSupportClass->cppIncludes);
            writeLinesToFile(supportClassCpp, pSupportClass->cppStatements);
            supportClassCpp.close();
          }
        });
    }
  }
  catch(const std::exception& ex)
  {
    Utils::Logger::logErr(errMsg + ": " + std::string(ex.what()));
  }
  catch(...)
  {
    Utils::Logger::logErr(errMsg);
  }
}
