/**
 *  A simple file reader.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2013-12-26
 */

#ifndef FILE_READER_HPP
#define FILE_READER_HPP

//System includes:
#include <string>

class Reader
{
public:
  Reader() : m_filePath(), m_fileText() {}
  static bool read(const std::string& filePath) { return get().readFile(filePath); }
  static std::string getFilePath() { return get().filePath(); }
  static std::string getFileText() { return get().fileText(); }
  static Reader& get() { static Reader reader; return reader; }
private:
  bool readFile(const std::string& filePath);
  std::string filePath() const { return m_filePath; }
  std::string fileText() const { return m_fileText; }
private:
  std::string m_filePath;
  std::string m_fileText;
};

#endif //FILE_READER_HPP
