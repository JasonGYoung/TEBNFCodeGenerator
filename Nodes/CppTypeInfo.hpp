/**
 *  C++ type information
 *
 * @author  Jason Young
 * @version 0.1
 */

#ifndef CPPTYPEINFO_HPP
#define CPPTYPEINFO_HPP

#include "../Utils/Optional.hpp"

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class Node;
class Element;
class SubElement;
struct Token;

namespace
{
  static const std::string DATA_OFFSET("rDataOffset");
  static const std::string MARSHAL_OFFSET("offset");
  static const std::string UNMARSHAL_OFFSET("destOffset");
}

enum VectorType_e
{
  TYPE_VEC_UNKNOWN,
  TYPE_VEC_BOUNDED,
  TYPE_VEC_MINMAX_BOUNDED,
  TYPE_VEC_UNBOUNDED,
  TYPE_VEC_MIN_UNBOUNDED,
  TYPE_VEC_MAX_UNBOUNDED
};

struct TypeRange
{
  size_t minValue;
  size_t maxValue;
  std::shared_ptr<Token> pMinToken;
  std::shared_ptr<Token> pMaxToken;
  VectorType_e vectorType;

  TypeRange()
  : minValue(0),
    maxValue(0),
    pMinToken(),
    pMaxToken(),
    vectorType(TYPE_VEC_UNKNOWN)
  {}

  TypeRange(std::shared_ptr<SubElement> pSubElement,
            const std::vector<std::shared_ptr<Token> >& tokens);

  TypeRange(size_t minVal, size_t maxVal);

  size_t getSize() const { return maxValue >= minValue ? maxValue : minValue; ; }
  std::string getMaxRangeStr();
  std::string getMinRangeStr();

  bool isVectorUnbounded() { return TYPE_VEC_UNBOUNDED == vectorType || TYPE_VEC_MAX_UNBOUNDED == vectorType; }

}; //end struct TypeRange

struct SupportClass
{
  std::string typeName;
  std::vector<std::string> cppIncludes;
  std::vector<std::string> cppStatements;
  std::vector<std::string> hppIncludes;
  std::vector<std::string> hppStatements;
  SupportClass() : typeName() {}
  explicit SupportClass(const std::string& name) : typeName(name) {}
};

struct CppTypeInfo
{
  Node* pCppTypeNode;
  std::string typeStr;
  std::string typeNameStr;
  bool isLiteralVal;
  bool isNumStr;
  bool isTypeAssignment;
  std::shared_ptr<SubElement> pTypeSizeCastSubElement;
  size_t typeSizeBits;
  size_t resolvedSizeBytes; //The total size of this type.
  size_t diffBytes;
  size_t resolvedSizeBits;
  std::shared_ptr<TypeRange> pTypeRange;
  std::vector<std::string> cppIncludes;
  std::vector<std::string> cppStatements;
  std::vector<std::string> hppIncludes;
  std::vector<std::string> hppStatements;

  bool isCppMain;

  std::vector<std::shared_ptr<SupportClass> > supportClasses;

  explicit CppTypeInfo()
    : pCppTypeNode(NULL),
    typeStr(),
    typeNameStr(),
    isLiteralVal(false),
    isNumStr(false),
    isTypeAssignment(false),
    pTypeSizeCastSubElement(),
    typeSizeBits(0),
    resolvedSizeBytes(0),
    diffBytes(0),
    resolvedSizeBits(0),
    pTypeRange(),
    cppIncludes(),
    cppStatements(),
    hppIncludes(),
    hppStatements(),
    isCppMain(false),
    supportClasses()
  {}

  bool isString() const { return this && typeStr == "std::string"; }
  bool isBitset() const { return this && typeStr == "std::bitset" && pTypeRange && pTypeRange->vectorType != TYPE_VEC_UNKNOWN; }
  bool isVector() const { return this && !isBitset() && !isString() && pTypeRange && pTypeRange->vectorType != TYPE_VEC_UNKNOWN; }
  bool isLiteral() const { return this && isLiteralVal; }
  bool isStaticVariable() const;
  bool isType() const { return this && isTypeAssignment; }

  std::string getSizeStr();

  std::string getMarshalCall(
    const std::string& typeName = "",
    const std::string& dataBuf = "rData");

  std::string getUnmarshalCall(
    const std::string& typeName = "",
    const std::string& offsetVarName = DATA_OFFSET);

  std::string getMember(Utils::optional<std::string> optTypeName,
                        const std::string& lineEnd = ";\n")
  {
    if(!optTypeName || optTypeName.get().empty())
      return typeNameStr + lineEnd;
    return *optTypeName + "::" + typeNameStr + lineEnd;
  }
  
  std::string getTypeNameStr(bool typeQualify = true);

  void appendAccessedElementIncludes();

  void writeCodeToDisk(const std::string& dirPath);

};

#endif //CPPTYPEINFO_HPP
