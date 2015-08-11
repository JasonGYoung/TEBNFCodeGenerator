/**
 *  TEBNF Scanner.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2013-12-26
 */

//Primary include:
#include "Scanner.hpp"

//Local includes:
#include "Token.hpp"
#include "Tokens.hpp"
#include "Utils/Utils.hpp"

//System includes:
#include <algorithm>
#include <iostream>
#include <locale>
#include <regex>
#include <sstream>
#include <deque>
#include <vector>

namespace
{
  Token::TokenType currentElementType = Token::TYPE_NONE;
  Token::TokenType lastElementType = Token::TYPE_NONE;
  /** Contains tokens read from file. */
  std::vector<std::shared_ptr<Token> > m_tokenList;
  std::shared_ptr<Tokens> m_pTokens;
  bool isMultiLineComment = false;

  bool isIdentifierChar(char c)
  {
    std::locale loc;
    return std::isalnum(c, loc) || '_' == c;
  }

  bool isMatch(const std::string& tokenString,
               const std::string& inStr,
               std::shared_ptr<Token>& pToken,
               size_t& rPos)
  {
    if(!tokenString.empty() && !inStr.empty() && inStr.find(tokenString) == 0)
    {
      pToken->text = tokenString;
      rPos += pToken->text.size();
      return true;
    }
    return false;
  }

  bool isMatch(const std::string* pTokenStrings,
               const std::string& inStr,
               std::shared_ptr<Token>& pToken,
               size_t& rPos)
  {
    if(pTokenStrings && !inStr.empty())
    {
      for(size_t i = 0; !pTokenStrings[i].empty(); i++)
      {
        if(isMatch(pTokenStrings[i], inStr, pToken, rPos))
          return true;
      }
    }
    return false;
  }

  void skipWhitespace(std::string& rStr)
  {
    std::locale loc;
    size_t i = 0;
    while(i < rStr.length() && std::isspace(rStr[i], loc))
      i++;
    if(i < rStr.length())
      rStr = rStr.substr(i);
    else
      rStr.clear();
  }

  size_t skipToNextChar(std::string& rStr, char c)
  {
    size_t i = 0;
    while(i < rStr.length() && c != rStr[i])
      i++;
    if(i < rStr.length() && c == rStr[i])
      rStr = rStr.substr(i);
    else
      rStr.clear();
    return i;
  }

  bool isElement(const std::string& inStr,
                 std::shared_ptr<Token>& pToken,
                 size_t& rPos)
  {
    if(isMatch(std::string("GRAMMAR"), inStr, pToken, rPos))
      pToken->type = Token::ELEMENT_GRAMMAR;
    else if(isMatch(std::string("INPUT"), inStr, pToken, rPos))
      pToken->type = Token::ELEMENT_INPUT;
    else if(isMatch(std::string("OUTPUT"), inStr, pToken, rPos))
      pToken->type = Token::ELEMENT_OUTPUT;
    else if(isMatch(std::string("STATES"), inStr, pToken, rPos))
      pToken->type = Token::ELEMENT_STATE_TABLE;
    else if(isMatch(std::string("ACTIONS"), inStr, pToken, rPos))
      pToken->type = Token::ELEMENT_ACTIONS;
    else
      return false;
    lastElementType = currentElementType;
    currentElementType = pToken->type;
    return true;
  }

  bool isElementName(const std::string& inStr,
                     std::shared_ptr<Token>& pToken,
                     size_t& rPos)
  {
    if(inStr.empty() || '@' != inStr[0])
      return false;
    std::locale loc;
    pToken->text = "@";
    rPos++;
    for(size_t i = 1; i < inStr.length() && isIdentifierChar(inStr[i]); i++)
    {
      pToken->text += inStr[i];
      rPos++;
    }
    if(rPos < inStr.length() && '.' == inStr[rPos])
      return false;
    pToken->type = Token::ELEMENT_NAME;
    return true;
  }

  bool isElementEnd(const std::string& inStr,
                    std::shared_ptr<Token>& pToken,
                    size_t& rPos)
  {
    if(!isMatch(std::string("END"), inStr, pToken, rPos))
      return false;
    pToken->type = Token::ELEMENT_END;
    lastElementType = currentElementType;
    currentElementType = Token::TYPE_NONE;
    return true;
  }

  bool isActionsFuncCall = false;
  bool isActionsWithParam = false;
  bool isActionsParamDelim(const std::string& inStr,
                           std::shared_ptr<Token>& pToken,
                           size_t& rPos)
  {
    if(Token::ELEMENT_ACTIONS != currentElementType || m_tokenList.size() < 3)
      return false;
    isActionsFuncCall = true;
    size_t i = m_tokenList.size()-1;
    auto pLastTok = Utils::getItemAt(m_tokenList, i);
    if(!isActionsWithParam &&
       Utils::getItemAt(m_tokenList, i-2)->isElementActions() &&
       Utils::getItemAt(m_tokenList, i-1)->isElementName())
    {
      if(!isActionsWithParam)
      {
        isActionsWithParam = pLastTok->isOperatorLeftParen();
        Utils::getItemAt(m_tokenList, i-1)->isActionsLastSignatureToken = !isActionsWithParam;
        isActionsFuncCall = false;
        return isActionsWithParam;
      }
    }
    else if(isActionsWithParam && pLastTok->isSymbolActionsParam() && isMatch(",", inStr, pToken, rPos))
    {
      pToken->type = Token::OPERATOR_ACTIONS_PARAM_DELIM;
      return true;
    }
    else if(isActionsWithParam && pLastTok->isSymbolActionsParam() && isMatch(")", inStr, pToken, rPos))
    {
      pToken->type = Token::OPERATOR_RIGHT_PAREN;
      pToken->isActionsLastSignatureToken = true;
      isActionsFuncCall = false;
      return true;
    }
    else if(isActionsWithParam && pLastTok->isOperatorRightParen())
    {
      isActionsWithParam = false;
      size_t i = m_tokenList.size()-1;
      std::vector<std::shared_ptr<Token> > signatureTokens;
      TokenUtils::getTokenRange(m_tokenList, signatureTokens, i,
        Token::OPERATOR_LEFT_PAREN, Token::OPERATOR_RIGHT_PAREN);
      if(!signatureTokens.empty())
      {
        for(size_t j = 0; j < signatureTokens.size(); j++)
        {
          auto pTok = Utils::getItemAt(signatureTokens, j);
          if(!pTok->isOperatorLeftParen() && !pTok->isOperatorRightParen())
          {
            if("," == pTok->text)
              pTok->type = Token::OPERATOR_ACTIONS_PARAM_DELIM;
            else if(!pTok->text.empty())
              pTok->type = Token::SYMBOL_ACTIONS_PARAM;
          }
        }
        Utils::getItemAt(m_tokenList, i)->isActionsLastSignatureToken = true;
        isActionsFuncCall = false;
        return true;
      }
    }
    isActionsFuncCall = false;
    return false;
  }

  bool isActionsParam(const std::string& inStr,
                      std::shared_ptr<Token>& pToken,
                      size_t& rPos)
  {
    if(!isActionsWithParam)
      return false;
    for(size_t i = 0; isIdentifierChar(inStr[i]) && i < inStr.length(); i++)
    {
      pToken->text += inStr[i];
      rPos++;
    }
    if(!pToken->text.empty())
    {
      size_t i = m_tokenList.size() - 1;
      auto pLastToken = Utils::getItemAt(m_tokenList, m_tokenList.size() - 1);
      if(pLastToken->isOperatorLeftParen() &&
         Utils::getItemAt(m_tokenList, i - 2)->isElementActions() &&
         Utils::getItemAt(m_tokenList, i - 1)->isElementName())
      {
        pToken->type = Token::SYMBOL_ACTIONS_PARAM;
        return true;
      }
      else if(pLastToken->isOperatorActionsParamDelim())
      {
        pToken->type = Token::SYMBOL_ACTIONS_PARAM;
        return true;
      }
    }
    return false;
  }

  bool isSubRuleClosure(const std::string& inStr,
                        std::shared_ptr<Token>& pToken,
                        size_t& rPos)
  {
    size_t i = rPos;
    if(Token::ELEMENT_GRAMMAR == currentElementType &&
       !isActionsWithParam &&
       Utils::getItemAt(m_tokenList, i)->isOperatorRightParen())
    {
      size_t i = m_tokenList.size()-1;
      std::vector<std::shared_ptr<Token> > closureTokens;
      TokenUtils::getTokenRange(m_tokenList, closureTokens, i,
        Token::OPERATOR_LEFT_PAREN, Token::OPERATOR_RIGHT_PAREN);
      if(!closureTokens.empty())
      {
        for(size_t j = 0; j < closureTokens.size(); j++)
        {
          auto pTok = Utils::getItemAt(closureTokens, j);
          if(!pTok->isOperatorLeftParen() && !pTok->isOperatorRightParen())
          {
            if("," == pTok->text)
              pTok->type = Token::OPERATOR_CONCAT;

          }
        }
      }
    }
    return false;
  }

  bool isRangeOrConcatOp(const std::string& inStr,
                         std::shared_ptr<Token>& pToken,
                         size_t& rPos)
  {
    if(isMatch(std::string(","), inStr, pToken, rPos) ||
       isMatch(std::string("}"), inStr, pToken, rPos))
    {
      //Back-track to determine what kind of comma this is.
      if(Token::ELEMENT_GRAMMAR != currentElementType)
        return false;
      for(size_t i = m_tokenList.size() - 1; i >= 0; i--)
      {
        bool isLitOrSymOrVar = m_tokenList.at(i)->isSymbolGrammar() ||
                               m_tokenList.at(i)->isStaticVariable() ||
                               m_tokenList.at(i)->isLiteral();
        if(isLitOrSymOrVar && (i-1) >= 0 && "{" == m_tokenList.at(i-1)->text)
        {
          pToken->type = Token::OPERATOR_RANGE_DELIM;
          m_tokenList.at(i-1)->type = Token::OPERATOR_RANGE_LEFT_BRACKET;
          m_tokenList.at(i)->actualType = m_tokenList.at(i)->type;
          m_tokenList.at(i)->type = Token::RANGE_VALUE;
          if("}" == pToken->text)
            pToken->type = Token::OPERATOR_RANGE_RIGHT_BRACKET;
          break;
        }
        else if(isLitOrSymOrVar && "}" == pToken->text)
        {
          pToken->type = Token::OPERATOR_RANGE_RIGHT_BRACKET;
          m_tokenList.at(i)->actualType = m_tokenList.at(i)->type;
          m_tokenList.at(i)->type = Token::RANGE_VALUE;
          break;
        }
        else if(Token::OPERATOR_RANGE_DELIM == m_tokenList.at(i)->type && "}" == pToken->text)
        {
          pToken->type = Token::OPERATOR_RANGE_RIGHT_BRACKET;
          break;
        }
        else if("," == pToken->text)
        {
          if("{" == m_tokenList.at(i)->text)
          {
            pToken->type = Token::OPERATOR_RANGE_DELIM;
            m_tokenList.at(i)->type = Token::OPERATOR_RANGE_LEFT_BRACKET;
          }
          else
            pToken->type = Token::OPERATOR_CONCAT;
          break;
        }
        else
          return false;
      }
      return true;
    }
    return false;
  }

  bool isTypeCast = false;
  bool isOperator(const std::string& inStr,
                  std::shared_ptr<Token>& pToken,
                  size_t& rPos)
  {
    static std::string lessGreatThanOps[] = {"<", "<=", ">", ">=", ""};
    static std::string compOps[] = {"==", "!=", ""};
    static std::string mathOps[] = {"+", "-", "*", "/", "%", ""};
    static std::string prodOps[] = {"{", "}", "(", ")", ""};
    if(isMatch(std::string("|"), inStr, pToken, rPos))
    {
      if(Token::ELEMENT_STATE_TABLE == currentElementType)
        pToken->type = Token::OPERATOR_STATE_TABLE_DELIM;
      else
        pToken->type = Token::OPERATOR_OR;
    }
    else if(isMatch(lessGreatThanOps, inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_LESS_GREATER_THAN_COMPARISON;
    else if(isMatch(compOps, inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_COMPARISON;
    else if(isMatch("AS", inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_AS;
    else if(isMatch("=", inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_ASSIGN;
    else if(isMatch("+=", inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_ASSIGN_INCREMENT;
    else if(isMatch("*=", inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_ASSIGN_MULTIPLY;
    else if(isMatch("/=", inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_ASSIGN_DIVIDE;
    else if(isMatch("%=", inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_ASSIGN_MODULO;
    else if(isMatch("-=", inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_ASSIGN_DECREMENT;
    else if(isMatch(std::string("--"), inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_DECREMENT;
    else if(isMatch(std::string("++"), inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_INCREMENT;
    else if(isMatch(mathOps, inStr, pToken, rPos))    
      pToken->type = Token::OPERATOR_MATH;
    else if(isMatch(std::string("."), inStr, pToken, rPos))
    {
      auto pLastToken = Utils::getItemAt(m_tokenList, m_tokenList.size()-1);
      auto tokenText = pLastToken->text;
      size_t pos = 0;
      if(isElementName(tokenText, pLastToken, pos))
        pLastToken->type = Token::SYMBOL_ELEMENT_NAME_ACCESSED;
      pToken->type = Token::OPERATOR_ELEMENT_MEMBER_ACCESS_DOT;
    }
    else if(isMatch(std::string("["), inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_ARRAY_LEFT_BRACKET;
    else if(isMatch(std::string("]"), inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_ARRAY_RIGHT_BRACKET;
    else if(isMatch(std::string("("), inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_LEFT_PAREN;
    else if(isMatch(std::string(")"), inStr, pToken, rPos))
    {
      if(isTypeCast)
        pToken->type = Token::OPERATOR_TYPE_CAST_RIGHT_PAREN;
      else
        pToken->type = Token::OPERATOR_RIGHT_PAREN;
    }
    else if(isMatch(prodOps, inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_PRODUCTION;
    else if(isMatch(std::string(";"), inStr, pToken, rPos))
      pToken->type = Token::OPERATOR_TERMINATION;
    else
      return false;
    return true;
  }

  bool isType(const std::string& inStr,
              std::shared_ptr<Token>& pToken,
              size_t& rPos)
  {
    static std::string types[] = {"BIT", "BYTE", "CHAR", ""};
    if(isMatch(types, inStr, pToken, rPos))
    {
      auto pLastToken = Utils::getItemAt(m_tokenList, m_tokenList.size() - 1);
      auto pLastLastToken = Utils::getItemAt(m_tokenList, m_tokenList.size() - 2);
      if(pLastToken->isOperatorLeftParen() && pLastLastToken->isOperatorAssignment())
      {
        pLastToken->type = Token::OPERATOR_TYPE_CAST_LEFT_PAREN;
        isTypeCast = true;
      }
      pToken->isUnsigned = "UNSIGNED" == Utils::getItemAt(m_tokenList, m_tokenList.size()-1)->text;
      if(pToken->isUnsigned)
        m_tokenList.pop_back();
      pToken->type = Token::SYMBOL_TYPED;
      return true;
    }
    static std::string sizedTypes[] = {"INT_STR", "FLOAT_STR", "INT", "FLOAT", ""};
    if(isMatch(sizedTypes, inStr, pToken, rPos))
    {
      auto pLastToken = Utils::getItemAt(m_tokenList, m_tokenList.size() - 1);
      auto pLastLastToken = Utils::getItemAt(m_tokenList, m_tokenList.size() - 2);
      if(pLastToken->isOperatorLeftParen() && pLastLastToken->isOperatorAssignment())
      {
        pLastToken->type = Token::OPERATOR_TYPE_CAST_LEFT_PAREN;
        isTypeCast = true;
      }
      if('_' == inStr[rPos])
      {
        pToken->text.append(1, inStr[rPos]);
        if(rPos+1 < inStr.length())
          rPos++;
        std::locale loc;
        for(size_t i = rPos; i < inStr.length() && std::isdigit(inStr[i], loc); i++)
        {
          pToken->text.append(1, inStr[i]);
          rPos++;
        }
      }
      pToken->isUnsigned = "UNSIGNED" == Utils::getItemAt(m_tokenList, m_tokenList.size()-1)->text;
      if(pToken->isUnsigned)
        m_tokenList.pop_back();
      pToken->type = Token::SYMBOL_TYPED;
      return true;
    }
    return false;
  }

  bool isLiteral(const std::string& inStr,
                 std::shared_ptr<Token>& pToken,
                 size_t& rPos)
  {
    std::locale loc;
    std::string token;    
    if(inStr.find("\"") == 0)
    {
      //Find string literal.
      for(size_t i = 1; i < inStr.length(); i++)
      {
        if('"' == inStr[i])
        {
          pToken->type = Token::LITERAL_STR;
          rPos = i + 1;
          break;
        }
      }
    }
    else if(inStr.find("'") == 0)
    {
      //Find char literal.
      if(inStr.length() > 2 && '\'' == inStr[2])
      {
        pToken->type = Token::LITERAL_CHR;
        rPos = 3;
      }
    }
    else
    {
      //See if this is a number.
      size_t i = 0;
      for(; i < inStr.length(); i++)
      {
        switch(inStr[i])
        {
        case 'x': case 'X':
          if(i == 1 && '0' == inStr[0])
            pToken->type = Token::LITERAL_HEX;
          break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
          if(Token::LITERAL_HEX != pToken->type)
            return false;
          break; //Consume and continue.
        case '.':
          if(i > 0 && std::isdigit(inStr[i-1], loc))
            pToken->type = Token::LITERAL_FLT;
          break;
        default:
          if(std::isdigit(inStr[i], loc))
          {
            if(Token::TYPE_NONE == pToken->type)
            {
              if(1 == i && '0' == inStr[0])
                pToken->type = Token::LITERAL_OCT;
              else
                pToken->type = Token::LITERAL_DEC;
            }
          }
          else if(Token::TYPE_NONE != pToken->type)          
            rPos = i;
          break;
        }
        if(rPos == i && i > 0)
          break;
      }
      if(Token::TYPE_NONE!= pToken->type)
        rPos = i;
    }
    if(Token::TYPE_NONE!= pToken->type)
    {
      pToken->text = inStr.substr(0, rPos);
      return true;
    }
    return false;
  }

  bool isCommentLine(const std::string& inStr,
                     std::shared_ptr<Token>& pToken,
                     size_t& rPos)
  {
    return inStr.find("##") == 0 || inStr.find("#") == 0 || isMultiLineComment;
  }

  bool isComment(const std::string& inStr,
                 std::shared_ptr<Token>& pToken,
                 size_t& rPos)
  {
    if(isMultiLineComment)
    {
      auto found = inStr.find("##");
      if(found == std::string::npos)
      {
        pToken->text = inStr;
        pToken->type = Token::COMMENT_MULTI_LINE;
        rPos += inStr.size();
      }
      else
      {
        isMultiLineComment = false;
        pToken->text = inStr.substr(0, found+2);
        pToken->type = Token::COMMENT_MULTI_LINE;
        rPos += found+2;
      }
      return true;
    }
    else if(inStr.find("##") == 0)
    {
      isMultiLineComment = true;
      pToken->text = inStr;
      pToken->type = Token::COMMENT_MULTI_LINE;
      rPos += inStr.size();
      return true;
    }
    else if(inStr.find("#") == 0)
    {
      pToken->text = inStr;
      pToken->type = Token::COMMENT_SINGLE_LINE;
      rPos += inStr.size();
      return true;
    }
    return false;
  }

  bool isStartsWithChar(const std::string& inStr,
                        std::shared_ptr<Token>& pToken,
                        size_t& rPos,
                        char startChar,
                        Utils::optional<Token::TokenType> optType = Utils::none)
  {
    if(!inStr.empty() && startChar == inStr[0])
    {
      std::locale loc;
      for(size_t i = 0; !std::isspace(inStr[i], loc) && i < inStr.length(); i++)
      {
        pToken->text += inStr[i];
        rPos++;
      }
      if(optType && !pToken->text.empty())
        pToken->type = *optType;
      return true;
    }
    return false;
  }

  bool isStaticVariable(const std::string& inStr,
                        std::shared_ptr<Token>& pToken,
                        size_t& rPos)
  {
    if(!inStr.empty() && '$' == inStr[0])
    {
      for(size_t i = 0;
          i < inStr.size() && ('$' == inStr[i] || isIdentifierChar(inStr[i]));
          i++, rPos++)
        pToken->text += inStr[i];
      pToken->type = Token::STATIC_VARIABLE;
      return true;
    }
    return false;
  }

  bool isSymbolGrammarToken(const std::string& inStr,
                            std::shared_ptr<Token>& pToken,
                            size_t& rPos)
  {
    if(Token::ELEMENT_GRAMMAR != currentElementType)
      return false;
    for(size_t i = 0; isIdentifierChar(inStr[i]) && i < inStr.length(); i++)
    {
      pToken->text += inStr[i];
      rPos++;
    }
    if(!pToken->text.empty())
    {
      auto pLastToken = Utils::getItemAt(m_tokenList, m_tokenList.size() - 1);
      if(pLastToken->isOperatorAssignment())
      {
        auto pLastLastToken = Utils::getItemAt(m_tokenList, m_tokenList.size() - 2);
        if(pLastLastToken->isOperatorTermination() || pLastLastToken->isElementName())
        {
          pToken->type = Token::SYMBOL_GRAMMAR_SIZE;
          return true;
        }
      }
      pToken->type = Token::SYMBOL_GRAMMAR;
      return true;
    }
    return false;
  }

  bool isSymbolConsoleToken(const std::string& inStr,
                            std::shared_ptr<Token>& pToken,
                            size_t& rPos)
  {
    if(Token::ELEMENT_INPUT != currentElementType &&
       Token::ELEMENT_OUTPUT != currentElementType)
      return false;
    for(size_t i = 0; isIdentifierChar(inStr[i]) && i < inStr.length(); i++)
    {
      pToken->text += inStr[i];
      rPos++;
    }
    if(!pToken->text.empty())
    {
      pToken->type = Token::SYMBOL_CONSOLE;
      return true;
    }
    return false;
  }

  bool isSymbolActionsToken(const std::string& inStr,
                            std::shared_ptr<Token>& pToken,
                            size_t& rPos)
  {
    if(Token::ELEMENT_ACTIONS != currentElementType)
      return false;
    for(size_t i = 0; isIdentifierChar(inStr[i]) && i < inStr.length(); i++)
    {
      pToken->text += inStr[i];
      rPos++;
    }
    if(!pToken->text.empty() && !pToken->isStaticVariable())
    {      
      pToken->type = Token::SYMBOL_ACTIONS;
      return true;
    }
    return false;
  }

  bool isSymbolStateTableToken(const std::string& inStr,
                               std::shared_ptr<Token>& pToken,
                               size_t& rPos)
  {
    if(Token::ELEMENT_STATE_TABLE != currentElementType)
      return false;
    for(size_t i = 0; isIdentifierChar(inStr[i]) && i < inStr.length(); i++)
    {
      pToken->text += inStr[i];
      rPos++;
    }
    if(!pToken->text.empty() && !pToken->isStaticVariable())
    {
      pToken->type = Token::SYMBOL_STATE_TABLE;
      return true;
    }
    return false;
  }

  bool isIoType(const std::string& inStr,
                std::shared_ptr<Token>& pToken,
                size_t& rPos)
  {
    static std::string types[] = {"TCP_IP", "UDP_IP", "FILE", "MYSQL", "GUI", "CONSOLE" ,""};
    if(isMatch(types, inStr, pToken, rPos) &&
       (Token::ELEMENT_INPUT == currentElementType ||
        Token::ELEMENT_OUTPUT == currentElementType))
    {
      pToken->type = Token::SYMBOL_IO_TYPE;
      return true;
    }
    return Token::SYMBOL_IO_TYPE == pToken->type;
  }

  
  bool isUnknownToken(const std::string& inStr,
                      std::shared_ptr<Token>& pToken,
                      size_t& rPos)
  {
    std::locale loc;
    if(pToken->text.empty())
    {
      for(size_t i = 0; !std::isspace(inStr[i], loc) && inStr[i] != '|' && inStr[i] != ';' && i < inStr.length(); i++)
      {
        pToken->text += inStr[i];
        rPos++;
      }
    }
    if(!pToken->text.empty())
    {

      switch(currentElementType)
      {
      case Token::ELEMENT_GRAMMAR: pToken->type = Token::SYMBOL_GRAMMAR; break;
      case Token::ELEMENT_ACTIONS: pToken->type = Token::SYMBOL_ACTIONS; break;
      case Token::ELEMENT_INPUT: break;
      case Token::ELEMENT_OUTPUT: break;
      case Token::ELEMENT_STATE_TABLE: break;
      default: pToken->type = Token::UNKNOWN; break;
      };
      return true;
    }
    return false;
  }

  void tokenizeLine(const std::string& inStr,
                    std::vector<std::shared_ptr<Token> >& tokens,
                    size_t lineNum)
  {
    auto pToken = std::make_shared<Token>();
    std::string s = inStr;
    size_t i = 0;
    while(i < s.length())
    {
      skipWhitespace(s);
      if(isComment(s, pToken, i))
        return;
      else if(isActionsParamDelim(s, pToken, i) ||
              isActionsParam(s, pToken, i) ||
              isRangeOrConcatOp(s, pToken, i) ||
              isIoType(s, pToken, i) ||
              isOperator(s, pToken, i) ||
              isElement(s, pToken, i) ||
              isElementName(s, pToken, i) ||
              isElementEnd(s, pToken, i) ||
              isType(s, pToken, i) ||
              isStaticVariable(s, pToken, i) ||
              isSymbolGrammarToken(s, pToken, i) ||
              isSymbolConsoleToken(s, pToken, i) ||
              isSymbolActionsToken(s, pToken, i) ||
              isSymbolStateTableToken(s, pToken, i) ||
              isLiteral(s, pToken, i) ||
              isUnknownToken(s, pToken, i))
      {
        pToken->lineNumber = lineNum;
        if(!pToken->text.empty())
        {
          pToken->index = tokens.size();
          bool isMemberAccess = !tokens.empty() && tokens.back()->isOperatorMemberAccessDot();
          if(isMemberAccess)
          {
            tokens.pop_back(); //Discard the dot token.
            pToken->pAccessedElementToken = tokens.back(); //Keep a copy of the element access token.
            tokens.pop_back(); //Discard the original element member access token.
            pToken->index = tokens.size();
          }
          tokens.push_back(pToken);
        }
        pToken = std::make_shared<Token>();
        if(i >= s.length())
          return;
        s = s.substr(i);
        if(!s.empty())
        {
          skipWhitespace(s);
          tokenizeLine(s, tokens, lineNum);
          return;
        }
      }
      i++;
    }
  }

  bool readTebnfLine(std::stringstream& inStrm, bool& rIsEnd, std::string& rOutStr)
  {
    while(true)
    {
      std::string lineText;
      std::getline(inStrm, lineText);
      if(!inStrm.good())
      {
        rIsEnd = true;
        return false;
      }
      skipWhitespace(lineText);
      auto pToken = std::make_shared<Token>();
      size_t pos = 0;
      if(isElement(lineText, pToken, pos) ||
         isCommentLine(lineText, pToken, pos) ||
         isMatch(std::string("END"), lineText, pToken, pos))
      {
        rOutStr = lineText;
        return true;
      }
      std::string currLine = lineText;
      size_t tebnfEndPos = Token::ELEMENT_ACTIONS == currentElementType ?
        skipToNextChar(currLine, ',') : skipToNextChar(currLine, ';');
      rOutStr.append(lineText);
      if((tebnfEndPos + 1) != lineText.length())
        return true;
    }
    return true;
  }
} //End of anonymous namespace

void Scanner::loadGrammar(const std::string& grammarText)
{
  Utils::Logger::log(Utils::getTabSpace() + "Loading grammar...", false);
  if(grammarText.empty())
    Utils::Logger::logErr("Empty grammar text");
  bool isEnd = false;
  std::stringstream strm(grammarText);
  while(!isEnd)
  {
    std::string lineText;
    for(size_t lineNum = 1; (readTebnfLine(strm, isEnd, lineText) && !isEnd); lineNum++)
    {
      tokenizeLine(lineText, m_tokenList, lineNum);
      lineText.clear();
    }
  }
  if(m_tokenList.empty())
    Utils::Logger::logErr("Failed to read anything");
  Utils::Logger::log(" finished");
}

std::shared_ptr<Tokens> Scanner::getTokensHelper() const
{
  if(!m_pTokens)
    m_pTokens = std::make_shared<Tokens>(m_tokenList);
  return m_pTokens;
}
