
//Primary include:
#include "Tokens.hpp"

std::shared_ptr<Token> Tokens::peekToken(size_t skipLen, SkipDirection_e direction)
{
  auto pTok = peekToken();
  if(!pTok)
    return std::shared_ptr<Token>();
  size_t idx = pTok->index;
  if(BACKWARD == direction)
  {
    if(idx >= skipLen)
      idx -= skipLen;
    else
      idx = 0;
  }
  else
  {
    if((idx + skipLen) < m_tokens.size())
      idx += skipLen;
    else
      idx = m_tokens.size() - 1;
  }
  return Utils::getItemAt(m_tokens, idx);
}

std::shared_ptr<Token> Tokens::nextToken()
{
  if(m_iter != m_tokens.end())
    m_pLastToken = *m_iter;
  ++m_iter;
  return (m_tokens.end() != m_iter) ? *m_iter : std::shared_ptr<Token>();
}

std::shared_ptr<Token> Tokens::seekToToken(size_t index)
{
  auto it = std::find_if(m_iter, m_tokens.end(),
    [&](std::shared_ptr<Token> pToken)
    { return index == pToken->index; });
  if(it != m_tokens.end())
  {
    m_iter = it;
    return *m_iter;
  }
  return std::shared_ptr<Token>();
}

std::vector<std::shared_ptr<Token> >
Tokens::getTokenRange(size_t startIdx, size_t endIdx)
{
  std::vector<std::shared_ptr<Token> > tokenRange;
  if(endIdx < m_tokens.size())
    tokenRange.assign(m_tokens.begin() + startIdx, m_tokens.begin() + endIdx);
  return tokenRange;
}
