/**
 *  TEBNF type utility functions.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-11-21
 */

#ifndef TYPEUTILS_HPP
#define TYPEUTILS_HPP

#include <memory>
#include <string>

struct CppTypeInfo;
class Node;
struct Token;

class TypeUtils
{
public:
  static bool getLiteralInfo(std::shared_ptr<Token> pTypeToken,
    std::string& rTypeStr,
    size_t& rTypeSizeBits,
    size_t& rResolvedSizeBytes);

  static std::shared_ptr<CppTypeInfo> getCppTypeInfo(Node* pTypeNode, Node* pNameNode);

  static bool getCppTypeInfo(std::shared_ptr<Token> pTypeToken, std::shared_ptr<CppTypeInfo>& pCppInfo);
};

#endif //TYPEUTILS_HPP
