/**
 * @see TypeUtils.hpp
 */

//Primary include:
#include "TypeUtils.hpp"

//Project includes:
#include "CppTypeInfo.hpp"
#include "Element.hpp"

bool TypeUtils::getLiteralInfo(
  std::shared_ptr<Token> pTypeToken,
  std::string& rTypeStr,
  size_t& rTypeSizeBits,
  size_t& rResolvedSizeBytes)
{
  if(!pTypeToken->isLiteral())
    return false;
  bool isUnsigned = pTypeToken && pTypeToken->isSymbolTypedUnsigned();
  int base = -1;
  std::string typeStr;
  size_t typeSizeBits = 0;
  switch(pTypeToken->type)
  {
  case Token::LITERAL_CHR: base = 10; break;
  case Token::LITERAL_OCT: base = 8; break;
  case Token::LITERAL_DEC: base = 10; break;
  case Token::LITERAL_HEX: base = 16; break;
  case Token::LITERAL_FLT:
  {
    auto num = Utils::stold(pTypeToken->text);
    typeStr = num <= FLT_MAX ? "float" :
      num <= DBL_MAX ? "double" :
      num <= LDBL_MAX ? "long double" :
      std::string();
    typeSizeBits = num <= FLT_MAX ? sizeof(float) * CHAR_BIT :
      num <= DBL_MAX ? sizeof(double) * CHAR_BIT :
      num <= LDBL_MAX ? sizeof(long double) * CHAR_BIT :
      0;
  }
    break;
  case Token::LITERAL_STR:
    typeStr = "std::string";
    rResolvedSizeBytes = pTypeToken->text.size() - 2; //subtract 2 for quotes.
    typeSizeBits = rResolvedSizeBytes * CHAR_BIT;
    break;
  default: return false;
  };
  if(base > -1)
  {
    auto num = Utils::stoll(pTypeToken->text, base);
    typeSizeBits = ((isUnsigned && num <= UCHAR_MAX) || num <= CHAR_MAX) ? CHAR_BIT :
      ((isUnsigned && num <= _UI16_MAX) || num <= _I16_MAX) ? 16 :
      ((isUnsigned && num <= _UI32_MAX) || num <= _I32_MAX) ? 32 :
      ((isUnsigned && num <= _UI64_MAX) || num <= _I64_MAX) ? 64 :
      0;
    switch(typeSizeBits)
    {
    case 8:  typeStr = isUnsigned ? "uint8_t" : "int8_t"; break;
    case 16: typeStr = isUnsigned ? "uint16_t" : "int16_t"; break;
    case 32: typeStr = isUnsigned ? "uint32_t" : "int32_t"; break;
    case 64: typeStr = isUnsigned ? "uint64_t" : "int64_t"; break;
    default: return false;
    };
    rTypeStr = typeStr;
    rTypeSizeBits = typeSizeBits;
    if(0 == rResolvedSizeBytes && typeSizeBits > 0)
      rResolvedSizeBytes = typeSizeBits / CHAR_BIT;
    return true;
  }
  else if(!typeStr.empty())
  {
    rTypeStr = typeStr;
    rTypeSizeBits = typeSizeBits;
    return true;
  }
  return false;
}

std::shared_ptr<CppTypeInfo> TypeUtils::getCppTypeInfo(Node* pTypeNode, Node* pNameNode)
{
  auto pCppInfo = std::make_shared<CppTypeInfo>();
  auto pTypeToken = pTypeNode->getToken();
  bool isStaticVariable = false;
  if(pNameNode)
  {
    pCppInfo->typeNameStr = Utils::getCppVarName(pNameNode->getToken()->text);
    isStaticVariable = pNameNode->getToken()->isStaticVariable();
  }  
  pCppInfo->isLiteralVal = getLiteralInfo(pTypeToken, pCppInfo->typeStr, pCppInfo->typeSizeBits, pCppInfo->resolvedSizeBytes);
  pCppInfo->isTypeAssignment = pTypeToken->isSymbolTyped();
  if(!pCppInfo->isLiteralVal && pCppInfo->isType())
  {
    getCppTypeInfo(pTypeToken, pCppInfo);
  }
  else if(isStaticVariable && pNameNode->hasChildren())
  {
    auto pChild = pNameNode->getChildren()->getChild(0);
    auto pElem = pChild->getContainingElement();
    auto pFoundChild = pElem->getChildren()->findChild(pChild->getToken()->text);
    auto pFoundCppTypeInfo = pFoundChild->getCppTypeInfo();
    if(pFoundCppTypeInfo)
    {
      std::string tempTypeNameStr = pCppInfo->typeNameStr;
      pCppInfo = std::make_shared<CppTypeInfo>(*pFoundCppTypeInfo);
      pCppInfo->typeNameStr = tempTypeNameStr;
      pCppInfo->pCppTypeNode = pNameNode;
    }
  }
  return pCppInfo;
}

bool TypeUtils::getCppTypeInfo(std::shared_ptr<Token> pTypeToken, std::shared_ptr<CppTypeInfo>& pCppInfo)
{
  if(!pTypeToken || !pTypeToken->isSymbolTyped())
    return false;
  if(!pCppInfo)
    pCppInfo = std::make_shared<CppTypeInfo>();
  bool isUnsigned = pTypeToken->isSymbolTypedUnsigned();
  std::string typeStr(pTypeToken->text);
  auto pos = typeStr.find_last_of("_");
  if(pos != std::string::npos)
  {
    std::string typeVal = typeStr.substr(0, pos);
    std::string num = typeStr.substr(pos + 1);
    pCppInfo->typeSizeBits = static_cast<size_t>(Utils::stoll(num));
    pCppInfo->resolvedSizeBytes = pCppInfo->typeSizeBits / CHAR_BIT;
    if(pCppInfo->typeSizeBits > 64)
    {
      pCppInfo->diffBytes = pCppInfo->resolvedSizeBytes - 8;
  //    Utils::Logger::logWarn(pTypeToken->lineNumber, "Conversion from " + num + " bits to 64 bits, possible loss of data in generated code");
      num = "64";
    }
    if(isUnsigned)
      typeVal = "u" + typeVal;
    pCppInfo->isNumStr = typeVal.find("INT_STR") != std::string::npos ||
      typeVal.find("FLOAT_STR") != std::string::npos;
    if(pCppInfo->isNumStr)
    {
      auto strPos = typeVal.find("_STR");
      if(strPos != std::string::npos)
        typeVal = typeVal.erase(strPos, 4);
    }
    pCppInfo->typeStr =
      ("FLOAT" == typeVal && pCppInfo->resolvedSizeBytes <= sizeof(float)) ? pCppInfo->typeStr = "float" :
      ("FLOAT" == typeVal && pCppInfo->resolvedSizeBytes <= sizeof(double)) ? pCppInfo->typeStr = "double" :
      Utils::trimCopy(typeVal + num + "_t");
    Utils::toLower(pCppInfo->typeStr);
  }
  else if(typeStr.find("BYTE") != std::string::npos)
  {
    pCppInfo->typeSizeBits = CHAR_BIT;
    pCppInfo->typeStr = isUnsigned ? "uint8_t" : "int8_t";
  }
  else if(typeStr.find("BIT") != std::string::npos)
  {
    pCppInfo->typeSizeBits = 1;
    pCppInfo->typeStr = "std::bitset";
  }
  else
    return false;
  return true;
}
