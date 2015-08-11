/**
 *  TEBNF File Reader.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2013-12-26
 */

//Primary include:
#include "Reader.hpp"

//Local includes:
#include "Utils/Utils.hpp"

//System includes:
#include <fstream>
#include <iostream>

bool Reader::readFile(const std::string& filePath)
{
  try
  {
    Utils::Logger::log(Utils::getTabSpace() + "Reading \"" + filePath + "\"...");
    m_filePath = filePath;
    std::ifstream fin(filePath, std::ifstream::binary);
    fin.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    if(fin)
    {
      fin.seekg(0, std::ios::end);
      m_fileText.resize(static_cast<size_t>(fin.tellg()));
      fin.seekg(0, std::ios::beg);
      fin.read(&m_fileText[0], m_fileText.size());
      fin.close();
      return true;
      Utils::Logger::log(" finished");
    }
    Utils::Logger::logErr("Failed to open " + filePath);
  }
  catch(const std::ifstream::failure& ifex)
  {
    Utils::Logger::logErr("Failed to open " + filePath + ": " + std::string(ifex.what()));
  }
  catch(const std::exception& ex)
  {
    Utils::Logger::logErr("Failed to open " + filePath + ": " + std::string(ex.what()));
  }
  catch(...)
  {
    Utils::Logger::logErr("Failed to open " + filePath);
  }
  return false;
}
