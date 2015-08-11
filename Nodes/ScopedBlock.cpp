
#include "ScopedBlock.hpp"
#include "../Utils/Utils.hpp"

ScopedBlock::ScopedBlock(std::stringstream& rStream,
                         size_t& rTabCount,
                         size_t newLineEndCount)
: m_rStream(rStream),
  m_rTabCount(rTabCount),
  m_newLineEndCount(newLineEndCount)
{
  m_rStream << Utils::getTabSpace(m_rTabCount) << "{\n";
  m_rTabCount++;
}

ScopedBlock::ScopedBlock(std::stringstream& rStream,
                         size_t& rTabCount,
                         const std::string& beginBlock,
                         size_t newLineEndCount)
: m_rStream(rStream),
  m_rTabCount(rTabCount),
  m_newLineEndCount(newLineEndCount)
{
  m_rStream << Utils::getTabSpace(m_rTabCount) << beginBlock << "\n";
  m_rStream << Utils::getTabSpace(m_rTabCount) << "{\n";
  m_rTabCount++;
}

ScopedBlock::~ScopedBlock()
{
  m_rTabCount--;
  size_t newLineEndCount = m_newLineEndCount > 0 ? m_newLineEndCount : 1; //Ensure its always > 0
  m_rStream << Utils::getTabSpace(m_rTabCount) << "}" << std::string(newLineEndCount, '\n');
}

std::ostream& ScopedBlock::operator<<(const std::string& line)
{
  m_rStream << Utils::getTabSpace(m_rTabCount) << line;
  return m_rStream;
}

