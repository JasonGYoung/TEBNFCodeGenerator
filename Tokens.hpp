/**
 *  Tokens container.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-09-09
 */

#ifndef TOKENS_HPP
#define TOKENS_HPP

//Local includes:
#include "Nodes/Element.hpp"
#include "Token.hpp"

//System includes:
#include <memory>
#include <vector>

class Tokens
{
public:
  enum SkipDirection_e { FORWARD, BACKWARD };
  explicit Tokens(const std::vector<std::shared_ptr<Token> >& tokens)
  : m_tokens(tokens),
    m_iter(m_tokens.begin()),
    m_pLastToken()
  {}
  std::shared_ptr<Token> peekToken() { return m_tokens.end() != m_iter ? *m_iter : std::shared_ptr<Token>(); }
  std::shared_ptr<Token> peekToken(size_t skipLen, SkipDirection_e direction);
  std::shared_ptr<Token> nextToken();
  std::shared_ptr<Token> seekToToken(size_t index);
  std::shared_ptr<Token> lastToken() { return m_pLastToken; }
  std::vector<std::shared_ptr<Token> > getTokenRange(size_t startIdx, size_t endIdx);
private:
  std::vector<std::shared_ptr<Token> > m_tokens;
  std::vector<std::shared_ptr<Token> >::iterator m_iter;
  std::shared_ptr<Token> m_pLastToken;
};

#endif //TOKENS_HPP
