/**
 *  TEBNF Parser.  Performs the following tasks:
 *  1) Syntax checking.
 *  2) Generates a parse tree suitable for traversal by the Input Tokenizer
 *     Generator.
 *
 *  WARNING: This class does NOT do a lot of logic checking i.e. making sure
 *  the logic of an application is correct.  It is impossible for this code to
 *  anticipate the true intentions of a developer beyond what has been expressed
 *  within the TEBNF grammar.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-01-03
 */

#ifndef PARSER_HPP
#define PARSER_HPP

//Local includes:
#include "../Nodes/Element.hpp"
#include "../Scanner.hpp"
#include "../Tokens.hpp"
#include "../Utils/Utils.hpp"

//System includes:
#include <memory>
#include <map>
#include <vector>
#include <sstream>

class Parser
{
public:
  Parser() : m_pCurrentElement() {}
  static void parse() { get().parseTokens(); }
  static Parser& get() { static Parser parser; return parser; }
private:
  void parseTokens();
  bool validateNewElement(std::shared_ptr<Element>& pElement);
  bool parseNewElement(std::shared_ptr<Element>& pElement,
                       std::shared_ptr<Tokens> pTokens);
private:
  std::shared_ptr<Element> m_pCurrentElement;
  size_t m_lastDelimIndex;
};

#endif //PARSER_HPP
