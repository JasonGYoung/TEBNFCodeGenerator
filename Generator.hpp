
#ifndef GENERATOR_HPP
#define GENERATOR_HPP

//System includes:
#include <map>
#include <string>
#include <vector>

class Generator
{
public:
  static void generate(const std::string& dirPath, const std::string& appName);
private:
  static void generateConversionClass(const std::string& dirPath);
  static void generateStaticVariableClass(const std::string& dirPath);
  static void generateClangFormatFile(const std::string& dirPath);
  static void generateCMakelists(const std::string& dirPath, const std::string& appName);
  static void generateReport();
  static void reportGeneratedElementFile(const std::string& elementName, const std::string& generatedFileName);
  static std::map<std::string, std::vector<std::string> > m_elementFilesPerElementMap;
};

#endif //CODEGENERATOR_HPP
