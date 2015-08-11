/**
 *  Defines a token.
 *
 * @author  Jason Young
 * @version 0.1
 */

#ifndef TOKEN_HPP
#define TOKEN_HPP

//Local includes:
#include "Utils/Optional.hpp"

//System includes:
#include <memory>
#include <string>
#include <vector>

class Node;

struct Token
{
  static const size_t NO_PRECEDENCE = 99999;

  Node* pNode;

  /** Token type. */
  enum TokenType
  {
    TYPE_NONE = 0,
    COMMENT_SINGLE_LINE,
    COMMENT_MULTI_LINE,
    ELEMENT_ACTIONS,
    ELEMENT_GRAMMAR,
    ELEMENT_INPUT,
    ELEMENT_END,
    ELEMENT_NAME,
    ELEMENT_OUTPUT,
    ELEMENT_STATE_TABLE,
    LITERAL_CHR,
    LITERAL_DEC,
    LITERAL_FLT,
    LITERAL_HEX,
    LITERAL_OCT,
    LITERAL_STR,

    RANGE_VALUE,

    /* Beginning of operators */
    START_OF_OPERATORS,
    OPERATOR_COMPARISON,
    OPERATOR_LESS_GREATER_THAN_COMPARISON,
    OPERATOR_AS,
    OPERATOR_ASSIGN,
    OPERATOR_ASSIGN_INCREMENT,
    OPERATOR_ASSIGN_DECREMENT,
    OPERATOR_ASSIGN_MULTIPLY,
    OPERATOR_ASSIGN_DIVIDE,
    OPERATOR_ASSIGN_MODULO,
    OPERATOR_MATH,
    OPERATOR_DECREMENT,
    OPERATOR_INCREMENT,
    OPERATOR_PRODUCTION,
    OPERATOR_ARRAY_LEFT_BRACKET,
    OPERATOR_ARRAY_RIGHT_BRACKET,
    OPERATOR_ELEMENT_MEMBER_ACCESS_DOT,
    OPERATOR_CONCAT,
    OPERATOR_OR,
    OPERATOR_LEFT_PAREN,
    OPERATOR_RIGHT_PAREN,
    OPERATOR_RANGE_LEFT_BRACKET,
    OPERATOR_RANGE_RIGHT_BRACKET,
    OPERATOR_RANGE_DELIM,
    OPERATOR_TYPE_CAST_LEFT_PAREN,
    OPERATOR_TYPE_CAST_RIGHT_PAREN,
    OPERATOR_STATE_TABLE_DELIM,
    OPERATOR_ACTIONS_PARAM_DELIM,
    OPERATOR_TERMINATION,
    END_OF_OPERATORS,
    /* End of operators */
    STATIC_VARIABLE,
    SYMBOL_ACTIONS,
    SYMBOL_GRAMMAR,
    SYMBOL_GRAMMAR_SIZE,
    SYMBOL_CONSOLE,
    SYMBOL_ELEMENT_NAME_ACCESSED, //TRICKY: Should we change this?
    SYMBOL_IO_TYPE,
    SYMBOL_ACTIONS_PARAM,
    SYMBOL_STATE_TABLE,
    /* Start of types set by parser */
    SYMBOL_STATE_TABLE_STATE,
    SYMBOL_STATE_TABLE_INPUT_OR_CONDITION,
    SYMBOL_STATE_TABLE_INPUT_METHOD,
    SYMBOL_STATE_TABLE_NEXT_STATE,
    SYMBOL_STATE_TABLE_OUTPUT_OR_ACTION,
    SYMBOL_STATE_TABLE_OUTPUT_METHOD,    
    /* End of types set by parser */
    SYMBOL_TYPED,
    UNKNOWN
  } type, actualType;
  /** Token text. */
  std::string text;
  /** Line number in orginal TEBNF source code that the token was read from. */
  size_t lineNumber;
  /** Index of this token in it's container. */
  size_t index;
  /** Set if this is a symbol accessed using the dot operator. */
  std::shared_ptr<Token> pAccessedElementToken;
  /** Operator associativity. */
  enum OperatorAssociativity { ASSOC_NONE, ASSOC_LEFT, ASSOC_RIGHT };
  /** True if this token is last token comprising the ACTIONS element signature,
   *  which may or may not include parameters. */
  bool isActionsLastSignatureToken;
  /** True if this is an unsigned type. */
  bool isUnsigned;
  /** Constructor.
   */
  Token(TokenType tokType = TYPE_NONE,
        const std::string& tokText = "",
        size_t tokPrecedence = NO_PRECEDENCE,
        OperatorAssociativity tokAssociativity = ASSOC_NONE)
  : pNode(NULL),
    type(tokType),
    actualType(TYPE_NONE),
    text(tokText),
    lineNumber(0),
    index(0),
    isActionsLastSignatureToken(false),
    isUnsigned(false)
  {}
  /** @return true if this token is a single line comment or part of a
   *    multi-line comment.
   */
  bool isComment() const { return COMMENT_MULTI_LINE == type || COMMENT_SINGLE_LINE == type; }
  /** Determine if a token is an element.
   * @return true if this token is an element, false otherwise.
   */
  bool isElement() const
  {
    return ELEMENT_ACTIONS == type || ELEMENT_GRAMMAR == type ||
           ELEMENT_INPUT == type || ELEMENT_OUTPUT == type ||
           ELEMENT_STATE_TABLE == type;
  }
  /** Element check functions.
   * @return true if this token matches the type being checked, false otherwise.
   */
  bool isElementName() const { return ELEMENT_NAME == type; }
  bool isElementGrammar() const { return ELEMENT_GRAMMAR == type; }
  bool isElementInput() const { return ELEMENT_INPUT == type; }
  bool isElementOutput() const { return ELEMENT_OUTPUT == type; }
  bool isElementIo() const { return isElementInput() || isElementOutput(); }  
  bool isElementStateTable() const { return ELEMENT_STATE_TABLE == type; }
  bool isElementActions() const { return ELEMENT_ACTIONS == type; }
  bool isElementEnd() const { return ELEMENT_END == type; }
  /** Symbol/variable check functions.
   * @return true if this token matches the type being checked, false otherwise.
   */
  bool isSymbolActions() const { return SYMBOL_ACTIONS == type; }
  bool isSymbolActionsParam() const { return SYMBOL_ACTIONS_PARAM == type; }
  bool isSymbolTyped() const { return SYMBOL_TYPED == type; }
  bool isSymbolTypedUnsigned() const { return isSymbolTyped() && isUnsigned; }
  bool isSymbolGrammar() const { return SYMBOL_GRAMMAR == type; }
  bool isSymbolGrammarSize() const { return SYMBOL_GRAMMAR_SIZE == type; }
  bool isSymbolConsole() const { return SYMBOL_CONSOLE == type; }
  bool isSymbolElementNameAccessed() { return SYMBOL_ELEMENT_NAME_ACCESSED == type; }
  bool isSymbolIoType() const { return SYMBOL_IO_TYPE == type; }
  bool isSymbolStateTable() const { return SYMBOL_STATE_TABLE == type; }
  bool isSymbolStateTableState() const { return SYMBOL_STATE_TABLE_STATE == type; }
  bool isStaticVariable() const { return STATIC_VARIABLE == type; }

  bool isRangeValue() const { return RANGE_VALUE == type; }

  /** Determine if a token is an operator.
   * @return true if this token is an operator, false otherwise.
   */
  bool isOperator() const { return type > START_OF_OPERATORS && type < END_OF_OPERATORS; }
  /** Operator check functions.
   */
  bool isOperatorMath() const { return OPERATOR_MATH == type; }
  bool isOperatorLeftAssociative() const { return OPERATOR_MATH == type; }
  bool isOperatorRightAssociative() const { return OPERATOR_ASSIGN == type; }
  bool isOperatorLeftArrayBracket() const { return OPERATOR_ARRAY_LEFT_BRACKET == type; }
  bool isOperatorRightArrayBracket() const { return OPERATOR_ARRAY_RIGHT_BRACKET == type; }
  bool isOperatorLeftRangeBracket() const { return OPERATOR_RANGE_LEFT_BRACKET == type; }
  bool isOperatorRightRangeBracket() const { return OPERATOR_RANGE_RIGHT_BRACKET == type; }
  bool isOperatorRangeDelim() const { return OPERATOR_RANGE_DELIM == type; }
  bool isOperatorTypeCastLeftParen() const { return OPERATOR_TYPE_CAST_LEFT_PAREN == type; }
  bool isOperatorTypeCastRightParen() const { return OPERATOR_TYPE_CAST_RIGHT_PAREN == type; }
  bool isOperatorLeftParen() const { return OPERATOR_LEFT_PAREN == type; }
  bool isOperatorRightParen() const { return OPERATOR_RIGHT_PAREN == type; }
  bool isOperatorConcat() const { return OPERATOR_CONCAT == type; }
  bool isOperatorAs() const { return OPERATOR_AS == type; }
  bool isOperatorOr() const { return OPERATOR_OR == type; }
  bool isOperatorComparison() const { return OPERATOR_COMPARISON == type; }
  static bool isOperatorOr(TokenType t) { return OPERATOR_OR == t; }
  bool isOperatorActionsParamDelim() const { return OPERATOR_ACTIONS_PARAM_DELIM == type; }
  bool isOperatorStateTableDelimiter() const { return OPERATOR_STATE_TABLE_DELIM == type; }
  bool isOperatorTermination() const { return OPERATOR_TERMINATION == type; }
  bool isOperatorMemberAccessDot() const { return OPERATOR_ELEMENT_MEMBER_ACCESS_DOT == type; }
  static bool isOperatorAssignment(TokenType t, bool excludeIncDecMulDivMod = false)
  {
    return excludeIncDecMulDivMod ? OPERATOR_ASSIGN == t :
           (OPERATOR_ASSIGN == t ||
            OPERATOR_ASSIGN_INCREMENT == t ||
            OPERATOR_ASSIGN_DECREMENT == t ||
            OPERATOR_ASSIGN_MULTIPLY == t ||
            OPERATOR_ASSIGN_DIVIDE == t ||
            OPERATOR_ASSIGN_MODULO == t);
  }
  bool isOperatorAssignment(bool excludeIncAndDec = false) const
  { return Token::isOperatorAssignment(type); }

  bool isSeparator() const { return "," == text || "|" == text;  }
  bool isLiteral() const
  {
    return LITERAL_CHR == type || LITERAL_DEC == type || LITERAL_FLT == type ||
      LITERAL_HEX == type || LITERAL_OCT == type || LITERAL_STR == type;
  }
  bool isLiteralNumber() const { return LITERAL_STR != type && isLiteral(); }
  bool isLiteralString() const { return LITERAL_STR == type && isLiteral(); }

  /** Unknown/unassigned type check functions.
   */
  bool isTypeUnknown() { return UNKNOWN == type; }
  bool isTypeNone() { return TYPE_NONE == type; }
  bool isTypeUnknownOrNone() { return UNKNOWN == type || TYPE_NONE == type; }

  bool isLeftAssociative() { return ASSOC_LEFT == associativity(); }
  bool isRightAssociative() { return ASSOC_RIGHT == associativity(); }
  Token::OperatorAssociativity associativity()
  {
    if(!isOperator())
      return Token::ASSOC_NONE;
    return
      ("++" == text || "--" == text || "(" == text || ")" == text ||
       "[" == text || "]" == text || "{" == text || "}" == text || "."  == text ||
       "*" == text || "/" == text || "%" == text ||
       "+" == text || "-" == text ||
       "<" == text || "<=" == text || ">" == text || ">=" == text ||
       "==" == text || "!=" == text ||
       ",") ? Token::ASSOC_LEFT :
      ("=" == text || "+=" == text || "-=" == text || "*=" == text || "/=" == text) ? Token::ASSOC_RIGHT :
      Token::ASSOC_NONE;
  }

  size_t precedence()
  {
    if(!isOperator())
      return Token::NO_PRECEDENCE;
    return
      ("++" == text || "--" == text || "(" == text || ")" == text ||
      "[" == text || "]" == text || "{" == text || "}" == text || "."  == text) ? 2 :
      ("==" == text || "!=" == text) ? 4 :
      ("+" == text || "-" == text) ? 5 :
      ("*" == text || "/" == text || "%" == text) ? 6 :
      ("<" == text || "<=" == text || ">" == text || ">=" == text) ? 8 :
      ("=" == text || "+=" == text || "-=" == text) ? 15 :
      (",") ? 17 :
      Token::NO_PRECEDENCE;
  }

  static int cmpPrecedence(Token t1, Token t2);

  /** Reset values.
   */
  void reset()
  {
    text.clear();
    type = TYPE_NONE;
  }
  /** Operator overloads.
   */
  bool operator==(const Token& rRhs)
  { return this->text == rRhs.text && this->type == rRhs.type; }

}; //struct Token

namespace TokenUtils
{

  bool getTokenRange(const std::vector<std::shared_ptr<Token> >& srcTokens,
                     std::vector<std::shared_ptr<Token> >& rRetTokens,
                     size_t searchStartIndex,
                     const std::string& startTokenText,
                     const std::string& endTokenText);

  bool getTokenRange(const std::vector<std::shared_ptr<Token> >& srcTokens,
                     std::vector<std::shared_ptr<Token> >& rRetTokens,
                     size_t searchStartIndex,
                     Token::TokenType startTokenType,
                     Token::TokenType endTokenType,
                     bool searchBackwards = true);

}

#endif //TOKEN_HPP
