#ifndef SCOPEDBLOCK_HPP
#define SCOPEDBLOCK_HPP

#include <sstream>
#include <string>
#include <vector>

class ScopedBlock
{
public:
  ScopedBlock(std::stringstream& rStream, size_t& rTabCount, size_t newLineEndCount = 1);
  ScopedBlock(std::stringstream& rStream, size_t& rTabCount, const std::string& beginBlock, size_t newLineEndCount = 1);
  ~ScopedBlock();
  std::ostream& ScopedBlock::operator<<(const std::string& line);
private:
  std::stringstream& m_rStream;
  size_t& m_rTabCount;
  size_t m_newLineEndCount;
};

#endif // SCOPEDBLOCK_HPP
