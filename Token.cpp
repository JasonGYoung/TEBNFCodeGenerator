
//Primary include:
#include "Token.hpp"

//Local includes:
#include "Utils/Optional.hpp"
#include "Utils/Utils.hpp"

//System includes:
#include <memory>

int Token::cmpPrecedence(Token t1, Token t2)
{
  int p1 = static_cast<int>(t1.precedence());
  int p2 = static_cast<int>(t2.precedence());
  return p1 - p2;
}

namespace TokenUtils
{

  bool getTokenRange(const std::vector<std::shared_ptr<Token> >& srcTokens,
                     std::vector<std::shared_ptr<Token> >& rRetTokens,
                     size_t searchStartIndex,
                     const std::string& startTokenText,
                     const std::string& endTokenText,
                     bool searchBackwards)
  {
    size_t start = searchStartIndex;
    if(searchBackwards)
      for(; start > 0 && startTokenText != srcTokens.at(start)->text; --start);
    else
      for(; start < srcTokens.size() && startTokenText != srcTokens.at(start)->text; ++start);
    size_t end = searchStartIndex;
    if(searchBackwards)
      for(; end > 0 && end < srcTokens.size() && endTokenText != srcTokens.at(end)->text; --end);
    else
      for(; end < srcTokens.size() && endTokenText != srcTokens.at(end)->text; ++end);
    auto startIt = srcTokens.begin() + start;
    auto endIt = srcTokens.begin() + end+1;
    if(start >= end)
      return false;
    rRetTokens.resize((end+1)-start);
    std::copy(startIt, endIt, rRetTokens.begin());
    return !rRetTokens.empty();
  }

  bool getTokenRange(const std::vector<std::shared_ptr<Token> >& srcTokens,
                     std::vector<std::shared_ptr<Token> >& rRetTokens,
                     size_t searchStartIndex,
                     Token::TokenType startTokenType,
                     Token::TokenType endTokenType,
                     bool searchBackwards)
  {
    size_t start = searchStartIndex;
    if(searchBackwards)
      for(; start > 0 && startTokenType != srcTokens.at(start)->type; --start);
    else
      for(; start < srcTokens.size() && startTokenType != srcTokens.at(start)->type; ++start);
    size_t end = searchStartIndex;
    if(searchBackwards)
      for(; end > 0 && end < srcTokens.size() && endTokenType != srcTokens.at(end)->type; --end);
    else
      for(; end < srcTokens.size() && endTokenType != srcTokens.at(end)->type; ++end);
    if(start >= end || end >= srcTokens.size())
      return false;
    auto startIt = srcTokens.begin() + start;
    auto endIt = srcTokens.begin() + end+1;
    rRetTokens.resize((end+1)-start);
    std::copy(startIt, endIt, rRetTokens.begin());
    return !rRetTokens.empty();
  }

} //end namespace TokenUtils
