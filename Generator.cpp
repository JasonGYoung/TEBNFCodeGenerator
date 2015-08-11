//Primary include:
#include "Generator.hpp"

//Local includes:
#include "Nodes/Element.hpp"
#include "Nodes/Elements.hpp"

//System includes:
#include <iomanip>
#include <fstream>
#include <sstream>

std::map<std::string, std::vector<std::string> > Generator::m_elementFilesPerElementMap;

void Generator::generate(const std::string& dirPath, const std::string& appName)
{
  Utils::Logger::log(Utils::getTabSpace() + "Generating code...");
  size_t fileGenCount = 0;
  //Generate entry point for application.
  auto spc = Utils::getTabSpace();
  auto pMainInfo = std::make_shared<CppTypeInfo>();
  pMainInfo->typeNameStr = appName;
  pMainInfo->isCppMain = true;
  pMainInfo->cppIncludes.push_back("#include <iostream>\n");
  pMainInfo->cppStatements.push_back("\nint main(int argc, const char* argv[])\n");
  pMainInfo->cppStatements.push_back("{\n");
  pMainInfo->cppStatements.push_back(spc + "try\n");
  pMainInfo->cppStatements.push_back(spc + "{\n");

  size_t instanceCount = 1;
  //Generate code for each element.
  for(std::shared_ptr<Element> pElement : Elements::elements())
  {
    if(pElement->isUsedInStateTable())
    {
      pElement->generateCode();
      pElement->getCppTypeInfo()->writeCodeToDisk(dirPath);
      if(pElement->getToken()->isElementStateTable())
      {
        std::string elemTypeName(pElement->getCppTypeInfo()->getTypeNameStr(false));
        pMainInfo->cppIncludes.push_back("#include \"" + elemTypeName + ".hpp\"\n");
        std::ostringstream line;
        line << spc << spc << elemTypeName << " stateTable_" << instanceCount++ << ";\n";
        pMainInfo->cppStatements.push_back(line.str());
      }        
    }
  }

  pMainInfo->cppStatements.push_back(spc + "}\n");
  pMainInfo->cppStatements.push_back(spc + "catch(const std::exception& ex)\n");
  pMainInfo->cppStatements.push_back(spc + "{\n");
  pMainInfo->cppStatements.push_back(spc + spc + "std::cerr << \"An error occurred: \" << ex.what() << std::endl;\n");
  pMainInfo->cppStatements.push_back(spc + "}\n");
  pMainInfo->cppStatements.push_back(spc + "catch(...)\n");
  pMainInfo->cppStatements.push_back(spc + "{\n");
  pMainInfo->cppStatements.push_back(spc + spc + "std::cerr << \"An unknown error occurred\" << std::endl;\n");
  pMainInfo->cppStatements.push_back(spc + "}\n");
  pMainInfo->cppStatements.push_back(spc + "return 0;\n");
  pMainInfo->cppStatements.push_back("}\n");
  pMainInfo->writeCodeToDisk(dirPath);

  generateConversionClass(dirPath);
  generateStaticVariableClass(dirPath);
  generateClangFormatFile(dirPath);
  generateCMakelists(dirPath, appName);  

  generateReport();
}

void Generator::generateConversionClass(const std::string& dirPath)
{
  try
  {
    std::ofstream conversionFile(dirPath + "/Conversion.hpp", std::ios::out);
    std::string prefaceComment(Utils::getClassPrefaceComment(
      "Conversion.hpp",
      "Provides marshalling and unmarshalling support for grammar element classes."));
    conversionFile << prefaceComment
                   << "#ifndef CONVERSION_HPP\n"
                   << "#define CONVERSION_HPP\n\n"

                   << "#include <errno.h>\n"
                   << "#include <iostream>\n"
                   << "#include <stdint.h>\n"
                   << "#include <string>\n"
                   << "#include <type_traits>\n"
                   << "#include <vector>\n"
#if defined(_WIN32) || defined(_WIN64)
                   << "#include <winsock2.h>   //hton, ntoh, sockets\n\n"

                   << "#pragma comment(lib, \"Ws2_32.lib\") //Link with Ws2_32.lib\n\n"
#elif __unix
                   << "#include <netinet/in.h> //hton, ntoh\n"
                   << "#include <sys/socket.h> //sockets\n\n"
#endif
                   << "class Conversion\n"
                   << "{\n"
                   << "public:\n"
                   << "  static bool unmarshal(std::vector<uint8_t> data, uint8_t& rValue, size_t& rOffset)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = data[rOffset];\n"
                   << "    rOffset += 1;\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, uint8_t value, bool append)\n"
                   << "  {\n"
                   << "    if(append)\n"
                   << "     rData.insert(rData.end(), value);\n"
                   << "   else\n"
                   << "   {\n"
                   << "     rData.resize(1);\n"
                   << "     rData[0] = value;\n"
                   << "   }\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, int8_t& rValue, size_t& rOffset)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = data[rOffset];\n"
                   << "    rOffset += 1;\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, int8_t value, bool append)\n"
                   << "  {\n"
                   << "    if(append)\n"
                   << "      rData.insert(rData.end(), value);\n"
                   << "    else\n"
                   << "    {\n"
                   << "      rData.resize(1);\n"
                   << "      rData[0] = value;\n"
                   << "    }\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, uint16_t& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = ntohs(unmarshal<uint8_t, uint16_t>(data, rOffset, diffBytes));\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, uint16_t value, bool append)\n"
                   << "  {\n"
                   << "    marshal(rData, sizeof(uint16_t), htons(value), append);\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, int16_t& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = ntohs(unmarshal<uint8_t, int16_t>(data, rOffset, diffBytes));\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, int16_t value, bool append)\n"
                   << "  {\n"
                   << "    marshal(rData, sizeof(int16_t), htons(value), append);\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, uint32_t& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = ntohl(unmarshal<uint8_t, uint32_t>(data, rOffset, diffBytes));\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, uint32_t value, bool append)\n"
                   << "  {\n"
                   << "    marshal(rData, sizeof(uint32_t), htonl(value), append);\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, int32_t& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = ntohl(unmarshal<uint8_t, int32_t>(data, rOffset, diffBytes));\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, int32_t value, bool append)\n"
                   << "  {\n"
                   << "    marshal(rData, sizeof(int32_t), htonl(value), append);\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, uint64_t& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = ntohll(unmarshal<uint8_t, uint64_t>(data, rOffset, diffBytes));\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, uint64_t value, bool append)\n"
                   << "  {\n"
                   << "    marshal(rData, sizeof(uint64_t), htonll(value), append);\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, int64_t& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = ntohll(unmarshal<uint8_t, int64_t>(data, rOffset, diffBytes));\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, int64_t value, bool append)\n"
                   << "  {\n"
                   << "    marshal(rData, sizeof(int64_t), htonll(value), append);\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, float& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = unmarshalFloat<uint8_t, float, FloatConvert>(data, rOffset, diffBytes);\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, float value, bool append)\n"
                   << "  {\n"
                   << "    marshalFloat<uint8_t, float, FloatConvert>(rData, sizeof(float), value, append);\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, double& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    rValue = unmarshalFloat<uint8_t, double, DoubleConvert>(data, rOffset, diffBytes);\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, double value, bool append)\n"
                   << "  {\n"
                   << "    marshalFloat<uint8_t, double, DoubleConvert>(rData, sizeof(double), value, append);\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, std::string& rValue, size_t& rOffset, const std::string& expected = std::string(), size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + rValue.size()) && expected.empty()) return false;\n"
                   << "    if(!expected.empty() && data.size() >= expected.size())\n"
                   << "    {\n"
                   << "      std::string temp(reinterpret_cast<char*>(&data[rOffset]), expected.size());\n"
                   << "      if(temp == expected)\n"
                   << "      {\n"
                   << "        rValue = temp;\n"
                   << "        return true;\n"
                   << "      }\n"
                   << "    }\n"
                   << "    rValue.assign(reinterpret_cast<char*>(&data[rOffset]), data.size());\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, const std::string& value, bool append)\n"
                   << "  {\n"
                   << "    if(append) rData.insert(rData.end(), value.begin(), value.end());\n"
                   << "    else rData.assign(value.begin(), value.end());\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, std::vector<int8_t>& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + rValue.size())) return false;\n"
                   << "    size_t len = rValue.size();\n"
                   << "    auto startOffset = data.begin() + rOffset;\n"
                   << "    rValue.assign(startOffset, startOffset + len);\n"
                   << "    rOffset += len;\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, const std::vector<int8_t>& value, bool append)\n"
                   << "  {\n"
                   << "    if(append) rData.insert(rData.end(), value.begin(), value.end());\n"
                   << "    else rData.assign(value.begin(), value.end());\n"
                   << "  }\n\n"

                   << "  static bool unmarshal(std::vector<uint8_t> data, std::vector<uint8_t>& rValue, size_t& rOffset, size_t diffBytes = 0)\n"
                   << "  {\n"
                   << "    if(data.size() < (rOffset + rValue.size())) return false;\n"
                   << "    size_t len = rValue.size();\n"
                   << "    auto startOffset = data.begin() + rOffset;\n"
                   << "    rValue.assign(startOffset, startOffset + len);\n"
                   << "    rOffset += len;\n"
                   << "    return true;\n"
                   << "  }\n"
                   << "  static void marshal(std::vector<uint8_t>& rData, const std::vector<uint8_t>& value, bool append)\n"
                   << "  {\n"
                   << "    if(append) rData.insert(rData.end(), value.begin(), value.end());\n"
                   << "    else rData.assign(value.begin(), value.end());\n"
                   << "  }\n\n"

                   << "  template<typename T>\n"
                   << "  static bool unmarshalNumString(const std::vector<uint8_t>& data, T& rValue, size_t sizeOfNumStr, size_t& rOffset)\n"
                   << "  {\n"
                   << "    errno = 0; //To distinguish success or failure after strto_ call\n"
                   << "    if(data.size() < (rOffset + sizeof(rValue))) return false;\n"
                   << "    char* pEnd = nullptr;\n"
                   << "    std::vector<char> num(sizeOfNumStr);\n"
                   << "    memcpy(&num[0], reinterpret_cast<const char*>(&data[rOffset]), sizeOfNumStr);\n"
                   << "    if(std::is_floating_point<T>::value)\n"
                   << "    {\n"
                   << "      if(sizeOfNumStr == 1) rValue = static_cast<T>(data[rOffset]);\n"
                   << "      else if(sizeOfNumStr <= 4) rValue = static_cast<T>(strtof(&num[0], &pEnd));\n"
                   << "      else rValue = static_cast<T>(strtold(&num[0], &pEnd));\n"
                   << "    }\n"
                   << "    else\n"
                   << "    {\n"
                   << "      if(sizeOfNumStr == 1) rValue = static_cast<T>(data[rOffset]);\n"
                   << "      else if(sizeOfNumStr <= 4) rValue = static_cast<T>(strtol(&num[0], &pEnd, 10));\n"
                   << "      else rValue = static_cast<T>(strtoll(&num[0], &pEnd, 10));\n"
                   << "    }\n"
                   << "    rOffset += sizeOfNumStr;\n"
                   << "    return 0 == errno;\n"
                   << "  }\n"
                   << "  template<typename T>\n"
                   << "  static void marshalNumString(std::vector<uint8_t>& rData, T value, size_t sizeOfNumStr, bool append)\n"
                   << "  {\n"
                   << "    size_t offset = append ? rData.size() : 0;\n"
                   << "    rData.resize(append ? rData.size() + sizeOfNumStr : sizeOfNumStr);\n"
                   << "    std::string valStr(std::to_string(value));\n"
                   << "    if(valStr.length() < sizeOfNumStr)\n"
                   << "      valStr = std::string((sizeOfNumStr - valStr.length()), '0') + valStr;\n"
                   << "    memcpy(&rData[offset], valStr.c_str(), sizeOfNumStr);\n"
                   << "  }\n\n"

                   << "  static bool compare(uint8_t* pData, std::string val)\n"
                   << "  {\n"
                   << "    return 0 == memcmp(pData, val.c_str(), val.length());\n"
                   << "  }\n\n"

                   << "  static bool compare(uint8_t* pData, uint8_t val)\n"
                   << "  {\n"
                   << "    return 0 == memcmp(pData, &val, sizeof(uint8_t));\n"
                   << "  }\n"
                   << "  static bool compare(int8_t* pData, uint8_t val)\n"
                   << "  {\n"
                   << "    return Conversion::compare(reinterpret_cast<uint8_t*>(pData), val);\n"
                   << "  }\n"
                   << "  static bool compare(uint8_t* pData, int8_t val)\n"
                   << "  {\n"
                   << "    return 0 == memcmp(pData, &val, sizeof(uint8_t));\n"
                   << "  }\n"
                   << "  static bool compare(int8_t* pData, int8_t val)\n"
                   << "  {\n"
                   << "    return Conversion::compare(reinterpret_cast<uint8_t*>(pData), val);\n"
                   << "  }\n\n"

                   << "  static bool compare(uint8_t* pData, uint16_t val)\n"
                   << "  {\n"
                   << "    auto v = htons(val);\n"
                   << "    return 0 == memcmp(pData, &v, sizeof(uint16_t));\n"
                   << "  }\n"
                   << "  static bool compare(int8_t* pData, uint16_t val)\n"
                   << "  {\n"
                   << "    auto v = htons(val);\n"
                   << "    return 0 == memcmp(reinterpret_cast<uint8_t*>(pData), &v, sizeof(uint16_t));\n"
                   << "  }\n"
                   << "  static bool compare(uint8_t* pData, int16_t val)\n"
                   << "  {\n"
                   << "    auto v = htons(val);\n"
                   << "    return 0 == memcmp(pData, &v, sizeof(int16_t));\n"
                   << "  }\n"
                   << "  static bool compare(int8_t* pData, int16_t val)\n"
                   << "  {\n"
                   << "    auto v = htons(val);\n"
                   << "    return 0 == memcmp(reinterpret_cast<uint8_t*>(pData), &v, sizeof(int16_t));\n"
                   << "  }\n\n"

                   << "  static bool compare(uint8_t* pData, uint32_t val)\n"
                   << "  {\n"
                   << "    auto v = htonl(val);\n"
                   << "    return 0 == memcmp(pData, &v, sizeof(uint32_t));\n"
                   << "  }\n"
                   << "  static bool compare(int8_t* pData, uint32_t val)\n"
                   << "  {\n"
                   << "    auto v = htonl(val);\n"
                   << "    return 0 == memcmp(reinterpret_cast<uint8_t*>(pData), &v, sizeof(uint32_t));\n"
                   << "  }\n"
                   << "  static bool compare(uint8_t* pData, int32_t val)\n"
                   << "  {\n"
                   << "    auto v = htonl(val);\n"
                   << "    return 0 == memcmp(pData, &v, sizeof(int32_t));\n"
                   << "  }\n"
                   << "  static bool compare(int8_t* pData, int32_t val)\n"
                   << "  {\n"
                   << "    auto v = htonl(val);\n"
                   << "    return 0 == memcmp(reinterpret_cast<uint8_t*>(pData), &v, sizeof(int32_t));\n"
                   << "  }\n\n"

                   << "  static bool compare(uint8_t* pData, uint64_t val)\n"
                   << "  {\n"
                   << "    auto v = htonll(val);\n"
                   << "    return 0 == memcmp(pData, &v, sizeof(uint64_t));\n"
                   << "  }\n"
                   << "  static bool compare(int8_t* pData, uint64_t val)\n"
                   << "  {\n"
                   << "    auto v = htonll(val);\n"
                   << "    return 0 == memcmp(reinterpret_cast<uint8_t*>(pData), &v, sizeof(uint64_t));\n"
                   << "  }\n"
                   << "  static bool compare(uint8_t* pData, int64_t val)\n"
                   << "  {\n"
                   << "    auto v = htonll(val);\n"
                   << "    return 0 == memcmp(pData, &v, sizeof(int64_t));\n"
                   << "  }\n"
                   << "  static bool compare(int8_t* pData, int64_t val)\n"
                   << "  {\n"
                   << "    auto v = htonll(val);\n"
                   << "    return 0 == memcmp(reinterpret_cast<uint8_t*>(pData), &v, sizeof(int64_t));\n"
                   << "  }\n\n"

                   << "private:\n"

                   << "  union FloatConvert\n"
                   << "  {\n"
                   << "    float value = 0;\n"
                   << "    uint8_t bytes[sizeof(float)];\n"
                   << "  };\n\n"

                   << "  union DoubleConvert\n"
                   << "  {\n"
                   << "    double value = 0;\n"
                   << "    uint8_t bytes[sizeof(double)];\n"
                   << "  };\n\n"

                   << "  template<typename VEC_T, typename T>\n"
                   << "  static T unmarshal(std::vector<VEC_T> data, size_t& rOffset, size_t diffBytes)\n"
                   << "  {\n"
                   << "    size_t valSize = sizeof(T) + diffBytes;\n"
                   << "    if(valSize > data.size()) throw std::runtime_error(\"Offset > data size!\");\n"
                   << "    T val;\n"
                   << "    memcpy(&val, &data[rOffset], valSize);\n"
                   << "    rOffset += valSize;\n"
                   << "    return val;\n"
                   << "  }\n"
                   << "  template<typename VEC_T, typename T>\n"
                   << "  static void marshal(std::vector<VEC_T>& rData, size_t typeSize, T val, bool append)\n"
                   << "  {\n"
                   << "    rData.resize(append ? rData.size() + typeSize : typeSize);\n"
                   << "    memcpy(&rData[append ? rData.size() : 0], &val, typeSize);\n"
                   << "  }\n\n"

                   << "  template<typename VEC_T, typename FLT_T, typename FLT_NUM_CONVERT>\n"
                   << "  static FLT_T unmarshalFloat(std::vector<VEC_T> data, size_t& rOffset, size_t diffBytes)\n"
                   << "  {\n"
                   << "    size_t valSize = sizeof(FLT_T) + diffBytes;\n"
                   << "    if(valSize > data.size()) throw std::runtime_error(\"offset > data size!\");\n"
                   << "    FLT_NUM_CONVERT convert;\n"
                   << "    std::copy(data.begin(), data.begin() + valSize, convert.bytes);\n"
                   << "    rOffset += valSize;\n"
                   << "    return convert.value;\n"
                   << "  }\n"
                   << "  template<typename VEC_T, typename FLT_T, typename FLT_NUM_CONVERT>\n"
                   << "  static void marshalFloat(std::vector<VEC_T>& rData, size_t typeSize, FLT_T val, bool append)\n"
                   << "  {\n"
                   << "    rData.resize(append ? rData.size() + typeSize : typeSize);\n"
                   << "    FLT_NUM_CONVERT convert;\n"
                   << "    convert.value = val;\n"
                   << "    auto offset = append ? rData.size() : 0;\n"
                   << "    std::copy(convert.bytes + offset, convert.bytes + typeSize, rData.begin());\n"
                   << "  }\n\n"                  
                   << "};\n"
                   << "#endif //CONVERSION_HPP\n\n";

    conversionFile.close();
  }
  catch(const std::exception& ex)
  {
    Utils::Logger::logErr("An error occurred while writing Conversion.hpp: " + std::string(ex.what()));
  }
  catch(...)
  {
    Utils::Logger::logErr("An unknown error occurred while writing Conversion.hpp");
  }
}

void Generator::generateStaticVariableClass(const std::string& dirPath)
{
  std::string hppPrefaceComment(Utils::getClassPrefaceComment(
    "StaticVariable.hpp",
    "Implements a discriminated union based on the marshaling and unmarshaling functions in Conversion.hpp."));
  std::ofstream staticVariableFileHpp(dirPath + "/StaticVariable.hpp", std::ios::out);
  staticVariableFileHpp
    << hppPrefaceComment
    << "#ifndef STATICVARIABLE_HPP\n"
    << "#define STATICVARIABLE_HPP\n\n"

    << "#include \"Conversion.hpp\"\n"
    << "#include <iostream>\n"
    << "#include <type_traits>\n\n"

    << "#define SV_TO_SV_OP(op)                                                      \\\n"
    << "  StaticVariable& operator op (const StaticVariable& rhs)                    \\\n"
    << "  {                                                                          \\\n"
    << "    if(SV_NONE == m_type && SV_NONE != rhs.getType())                        \\\n"
    << "    {                                                                        \\\n"
    << "      m_type = rhs.getType();                                                \\\n"
    << "    }                                                                        \\\n"
    << "    if(SV_FLOAT == m_type)                                                   \\\n"
    << "    {                                                                        \\\n"
    << "      auto thisVal = get<float>();                                           \\\n"
    << "      auto rhsVal = rhs.get<float>();                                        \\\n"
    << "      set(thisVal op rhsVal);                                                \\\n"
    << "    }                                                                        \\\n"
    << "    else if(SV_DOUBLE == m_type)                                             \\\n"
    << "    {                                                                        \\\n"
    << "      auto thisVal = get<double>();                                          \\\n"
    << "      auto rhsVal = rhs.get<double>();                                       \\\n"
    << "      set(thisVal op rhsVal);                                                \\\n"
    << "    }                                                                        \\\n"
    << "    else if(SV_UINT == m_type || SV_INT == m_type)                           \\\n"
    << "    {                                                                        \\\n"
    << "      bool isUnsigned = SV_UINT == m_type;                                   \\\n"
    << "      if(m_typeSize == 1)                                                    \\\n"
    << "      {                                                                      \\\n"
    << "        auto thisVal = isUnsigned ? get<uint8_t>() : get<int8_t>();          \\\n"
    << "        auto rhsVal = isUnsigned ? rhs.get<uint8_t>() : rhs.get<int8_t>();   \\\n"
    << "        set(thisVal op rhsVal);                                              \\\n"
    << "      }                                                                      \\\n"
    << "      else if(m_typeSize == 2)                                               \\\n"
    << "      {                                                                      \\\n"
    << "        auto thisVal = isUnsigned ? get<uint16_t>() : get<int16_t>();        \\\n"
    << "        auto rhsVal = isUnsigned ? rhs.get<uint16_t>() : rhs.get<int16_t>(); \\\n"
    << "        set(thisVal op rhsVal);                                              \\\n"
    << "      }                                                                      \\\n"
    << "      else if(m_typeSize <= 4)                                               \\\n"
    << "      {                                                                      \\\n"
    << "        auto thisVal = isUnsigned ? get<uint32_t>() : get<int32_t>();        \\\n"
    << "        auto rhsVal = isUnsigned ? rhs.get<uint32_t>() : rhs.get<int32_t>(); \\\n"
    << "        set(thisVal op rhsVal);                                              \\\n"
    << "      }                                                                      \\\n"
    << "      else if(m_typeSize <= 8)                                               \\\n"
    << "      {                                                                      \\\n"
    << "        auto thisVal = isUnsigned ? get<uint64_t>() : get<int64_t>();        \\\n"
    << "        auto rhsVal = isUnsigned ? rhs.get<uint64_t>() : rhs.get<int64_t>(); \\\n"
    << "        set(thisVal op rhsVal);                                              \\\n"
    << "      }                                                                      \\\n"
    << "    }                                                                        \\\n"
    << "    else                                                                     \\\n"
    << "    {                                                                        \\\n"
    << "      throw std::runtime_error(\"operation requires an arithmetic type\");     \\\n"
    << "    }                                                                        \\\n"
    << "    return *this;                                                            \\\n"
    << "  }\n\n"

    << "#define T_TO_SV_OP(op)                       \\\n"
    << "  template<typename T>                       \\\n"
    << "  StaticVariable& operator op (const T& rhs) \\\n"
    << "  {                                          \\\n"
    << "    T thisVal = get<T>();                    \\\n"
    << "    set(thisVal op rhs);                     \\\n"
    << "    return *this;                            \\\n"
    << "  }\n\n"

    << "class StaticVariable\n"
    << "{\n"
    << "public:\n"
    << "  StaticVariable()\n"
    << "  : m_typeSize(0),\n"
    << "    m_valueAsString(),\n"
    << "    m_buffer(),\n"
    << "    m_type(SV_NONE)\n"
    << "  {}\n\n"

    << "  template<typename T>\n"
    << "  StaticVariable(const T& val)\n"
    << "  : m_typeSize(0),\n"
    << "    m_valueAsString(),\n"
    << "    m_buffer(),\n"
    << "    m_type(SV_NONE)\n"
    << "  {\n"
    << "    set(val);\n"
    << "  }\n\n"

    << "  template<typename T>\n"
    << "  void set(T val)\n"
    << "  {\n"
    << "    m_typeSize = sizeof(val);\n"
    << "    if(std::is_arithmetic<T>::value) m_valueAsString = std::to_string(val);\n"
    << "    if(std::is_integral<T>()) m_type = std::is_signed<T>() ? SV_INT : SV_UINT;\n"
    << "    Conversion::marshal(m_buffer, val, false);\n"
    << "  }\n\n"

    << "  void set(float val)\n"
    << "  {\n"
    << "    m_typeSize = sizeof(val);\n"
    << "    m_valueAsString = std::to_string(val);\n"
    << "    m_type = SV_FLOAT;\n"
    << "    Conversion::marshal(m_buffer, val, false);\n"
    << "  }\n\n"

    << "  void set(double val)\n"
    << "  {\n"
    << "    m_typeSize = sizeof(val);\n"
    << "    m_valueAsString = std::to_string(val);\n"
    << "    m_type = SV_DOUBLE;\n"
    << "    Conversion::marshal(m_buffer, val, false);\n"
    << "  }\n\n"

    << "  void set(const std::string& val)\n"
    << "  {\n"
    << "    m_typeSize = val.size();\n"
    << "    m_valueAsString = val;\n"
    << "    m_type = SV_STRING;\n"
    << "    Conversion::marshal(m_buffer, val, false);\n"
    << "  }\n\n"

    << "  template<typename T>\n"
    << "  T get() const\n"
    << "  {\n"
    << "    size_t offset = 0;\n"
    << "    switch(m_type)\n"
    << "    {\n"
    << "    case SV_FLOAT: return unmarshalByType<T, float>(offset); break;\n"
    << "    case SV_DOUBLE: return unmarshalByType<T, double>(offset); break;\n"
    << "    case SV_INT:\n"
    << "      {\n"
    << "        if(m_typeSize == 1) return unmarshalByType<T, int8_t>(offset);\n"
    << "        if(m_typeSize == 2) return unmarshalByType<T, int16_t>(offset);\n"
    << "        if(m_typeSize <= 4) return unmarshalByType<T, int32_t>(offset);\n"
    << "        if(m_typeSize <= 8) return unmarshalByType<T, int64_t>(offset);\n"
    << "      }\n"
    << "      break;\n"
    << "    case SV_UINT:\n"
    << "      {\n"
    << "        if(m_typeSize == 1) return unmarshalByType<T, uint8_t>(offset);\n"
    << "        if(m_typeSize == 2) return unmarshalByType<T, uint16_t>(offset);\n"
    << "        if(m_typeSize <= 4) return unmarshalByType<T, uint32_t>(offset);\n"
    << "        if(m_typeSize <= 8) return unmarshalByType<T, uint64_t>(offset);\n"
    << "      }\n"
    << "      break;\n"
    << "    };\n"
    << "    T val = 0;\n"
    << "    Conversion::unmarshal(m_buffer, val, offset);\n"
    << "    return val;\n"
    << "  }\n\n"

    << "  template<typename T> operator T() const { return get<T>(); }\n\n"

    << "  operator double() const { return get<double>(); }\n\n"

    << "  SV_TO_SV_OP(=)\n"
    << "  SV_TO_SV_OP(+)\n"
    << "  SV_TO_SV_OP(+=)\n"
    << "  SV_TO_SV_OP(-)\n"
    << "  SV_TO_SV_OP(-=)\n"
    << "  SV_TO_SV_OP(*)\n"
    << "  SV_TO_SV_OP(*=)\n"
    << "  SV_TO_SV_OP(/)\n"
    << "  SV_TO_SV_OP(/=)\n\n"

    << "  T_TO_SV_OP(=)\n"
    << "  T_TO_SV_OP(+)\n"
    << "  T_TO_SV_OP(+=)\n"
    << "  T_TO_SV_OP(-)\n"
    << "  T_TO_SV_OP(-=)\n"
    << "  T_TO_SV_OP(*)\n"
    << "  T_TO_SV_OP(*=)\n"
    << "  T_TO_SV_OP(/)\n"
    << "  T_TO_SV_OP(/=)\n"
    << "  T_TO_SV_OP(%)\n"
    << "  T_TO_SV_OP(%=)\n\n"

    << "  StaticVariable operator++()\n"
    << "  {\n"
    << "    switch(m_type)\n"
    << "    {\n"
    << "    case SV_FLOAT:\n"
    << "    case SV_DOUBLE: *this += 1.0f; break;\n"
    << "    case SV_INT:\n"
    << "    case SV_UINT: *this += 1; break;\n"
    << "    default: throw std::runtime_error(\"operation requires an arithmetic type\"); break;\n"
    << "    };\n"
    << "    return *this;\n"
    << "  }\n\n"

    << "  template<typename T>\n"
    << "  StaticVariable operator++(T)\n"
    << "  {\n"
    << "    StaticVariable temp(*this);\n"
    << "    operator++();\n"
    << "    return temp;\n"
    << "  }\n\n"

    << "  StaticVariable operator--()\n"
    << "  {\n"
    << "    switch(m_type)\n"
    << "    {\n"
    << "    case SV_FLOAT:\n"
    << "    case SV_DOUBLE: *this -= 1.0f; break;\n"
    << "    case SV_INT:\n"
    << "    case SV_UINT: *this -= 1; break;\n"
    << "    default: throw std::runtime_error(\"operation requires an arithmetic type\"); break;\n"
    << "    };\n"
    << "    return *this;\n"
    << "  }\n\n"

    << "  template<typename T>\n"
    << "  StaticVariable operator--(T)\n"
    << "  {\n"
    << "    StaticVariable temp(*this);\n"
    << "    operator--();\n"
    << "    return temp;\n"
    << "  }\n\n"

    << "  friend std::ostream& operator<<(std::ostream& os, const StaticVariable& rhs);\n\n"

    << "  enum StaticVariableType { SV_NONE, SV_UINT, SV_INT, SV_FLOAT, SV_DOUBLE, SV_STRING };\n"
    << "  StaticVariableType getType() const { return m_type; }\n"
    << "  std::string to_string() const { return m_valueAsString; }\n\n"
  
    << "private:\n"

    << "  template<typename RET_T, typename FROM_T>\n"
    << "  RET_T unmarshalByType(size_t& rOffset) const\n"
    << "  {\n"
    << "    FROM_T value = 0;\n"
    << "    Conversion::unmarshal(m_buffer, value, rOffset);\n"
    << "    return static_cast<RET_T>(value);\n"
    << "  }\n\n"

    << "private:\n"
    << "  size_t m_typeSize;\n"
    << "  std::string m_valueAsString;\n"
    << "  StaticVariableType m_type;\n"
    << "  std::vector<uint8_t> m_buffer;\n"
    << "};\n\n"

    << "#endif //STATICVARIABLE_HPP\n\n";

  std::string cppPrefaceComment(Utils::getClassPrefaceComment(
    "StaticVariable.cpp",
    "See description in Conversion.hpp."));
  std::ofstream staticVariableFileCpp(dirPath + "/StaticVariable.cpp", std::ios::out);
  staticVariableFileCpp
    << hppPrefaceComment
    << "#include \"StaticVariable.hpp\"\n\n"
    << "std::ostream& operator<<(std::ostream& os, const StaticVariable& rhs)\n"
    << "{\n"
    << "  os << rhs.to_string();\n"
    << "  return os;\n"
    << "}\n\n";
}

void Generator::generateClangFormatFile(const std::string& dirPath)
{
  try
  {
    std::ofstream clangFormatFile(dirPath + "/_clang-format", std::ios::out);
    clangFormatFile << "---\n"
      << "BasedOnStyle: LLVM\n"
      << "AccessModifierOffset: -2\n"
      << "AllowAllParametersOfDeclarationOnNextLine: true\n"
      << "AllowShortBlocksOnASingleLine: false\n"
      << "AllowShortFunctionsOnASingleLine: Inline\n"
      << "AllowShortIfStatementsOnASingleLine: true\n"
      << "AllowShortLoopsOnASingleLine: false\n"
      << "AlwaysBreakTemplateDeclarations: true\n"
      << "BinPackParameters: false\n"
      << "BreakBeforeBraces: Allman\n"
      << "BreakConstructorInitializersBeforeComma: false\n"
      << "ColumnLimit: 80\n"
      << "ConstructorInitializerAllOnOneLineOrOnePerLine: true\n"
      << "ConstructorInitializerIndentWidth: 2\n"
      << "ContinuationIndentWidth: 2\n"
      << "Cpp11BracedListStyle: true\n"
      << "DerivePointerAlignment: false\n"
      << "IndentCaseLabels: false\n"
      << "IndentWidth: 2\n"
      << "Language: Cpp\n"
      << "NamespaceIndentation: All\n"
      << "PenaltyBreakBeforeFirstCallParameter: 0\n"
      << "PenaltyBreakComment: 2000\n"
      << "PenaltyBreakString: 3000\n"
      << "PenaltyBreakFirstLessLess: 1000\n"
      << "PenaltyExcessCharacter: 100000\n"
      << "PenaltyReturnTypeOnItsOwnLine: 10000\n"
      << "PointerAlignment: Left\n"
      << "SpaceBeforeAssignmentOperators: true\n"
      << "SpaceBeforeParens: ControlStatements\n"
      << "SpaceInEmptyParentheses: false\n"
      << "SpacesInAngles: false\n"
      << "SpacesInParentheses: false\n"
      << "Standard: Cpp11\n"
      << "UseTab: Never\n";
    clangFormatFile.close();
  }
  catch(const std::exception& ex)
  {
    Utils::Logger::logErr("An error occurred while writing _clang-format: " + std::string(ex.what()));
  }
  catch(...)
  {
    Utils::Logger::logErr("An unknown error occurred while writing _clang-format");
  }
}

void Generator::generateCMakelists(const std::string& dirPath, const std::string& appName)
{
  try
  {
    std::ofstream cmakeFile(dirPath + "/CMakeLists.txt", std::ios::out);
    cmakeFile << "###############################################################\n"
      << "# Generated by TEBNF Code Generator v" << Utils::getTEBNFVersion() << "@" << Utils::getDateTimeAsString() << "\n"
      << "# Jason Young\n"
      << "cmake_minimum_required(VERSION 3.0)\n"
      << "project(\"" << appName << "\")\n"
      << "if(MSVC12)\n"
      << "  message(STATUS \"Compiler is compatible with code generated by the TEBNF Code Generator (C++11 MSVC compiler)\")\n"
      << "elseif(MSVC)\n"
      << "  message(WARNING \"Compiler may NOT be compatible with code generated by the TEBNF Code Generator (requires C++11 MSVC compiler)\")\n"
      << "elseif()\n"
      << "  message(FATAL_ERROR \"Code generated by the TEBNF Code Generator requires a C++11 MSVC compiler\")\n"
      << "endif()\n"
      << "###############################################################\n"
      << "# " << appName << "\n"
      << "add_executable(" << appName << "\n"
      << Utils::getTabSpace() << appName << "_Main" << ".cpp\n"
      << Utils::getTabSpace() << "Conversion.hpp\n"
      << Utils::getTabSpace() << "StaticVariable.cpp\n"
      << Utils::getTabSpace() << "StaticVariable.hpp\n";
    std::set<std::string> fileNames;
    for(std::shared_ptr<Element> pElement : Elements::elements())
    {
      if(pElement->isUsedInStateTable() && !pElement->isAsElement())
      {
        std::string cppName(pElement->getCppTypeInfo()->typeNameStr + ".cpp");
        if(fileNames.end() == fileNames.find(cppName) && !pElement->getCppTypeInfo()->cppStatements.empty())
        {
          fileNames.insert(cppName);
          reportGeneratedElementFile(pElement->getCppTypeInfo()->typeNameStr, cppName);
          cmakeFile << Utils::getTabSpace() << cppName << "\n";
        }
        std::string hppName(pElement->getCppTypeInfo()->typeNameStr + ".hpp");
        if(fileNames.end() == fileNames.find(hppName))
        {
          fileNames.insert(hppName);
          reportGeneratedElementFile(pElement->getCppTypeInfo()->typeNameStr, hppName);
          cmakeFile << Utils::getTabSpace() << hppName << "\n";
        }
      }
      auto pCppInfo = pElement->getCppTypeInfo();
      if(pCppInfo && !pCppInfo->supportClasses.empty())
      {
        for(std::shared_ptr<SupportClass> pSupportClass : pCppInfo->supportClasses)
        {
          std::string cppName(pSupportClass->typeName + ".cpp");
          if(!pSupportClass->cppStatements.empty() &&
             fileNames.end() == fileNames.find(cppName))
          {
            fileNames.insert(cppName);
            cmakeFile << Utils::getTabSpace() << cppName << "\n";
          }
          std::string hppName(pSupportClass->typeName + ".hpp");
          if(!pSupportClass->hppStatements.empty() &&
             fileNames.end() == fileNames.find(hppName))
          {
            fileNames.insert(hppName);
            cmakeFile << Utils::getTabSpace() << hppName << "\n";
          }
        }
      }
    }
    cmakeFile << Utils::getTabSpace() << ")\n";
    cmakeFile.close();
  }
  catch(const std::exception& ex)
  {
    Utils::Logger::logErr("An error occurred while writing CMakeLists.txt: " + std::string(ex.what()));
  }
  catch(...)
  {
    Utils::Logger::logErr("An unknown error occurred while writing CMakeLists.txt");
  }
}

void Generator::generateReport()
{
  size_t fileGenCount = 0;
  if(!m_elementFilesPerElementMap.empty()) std::cout << std::endl;
  size_t maxWidth = 0;
  for(auto it = m_elementFilesPerElementMap.begin(); it != m_elementFilesPerElementMap.end(); ++it)
  {
    for(std::string fileName : it->second)
    {
      std::string elemReport(it->first + " ");
      if(elemReport.size() > maxWidth)
        maxWidth = elemReport.size();
    }
  }
  for(auto it = m_elementFilesPerElementMap.begin(); it != m_elementFilesPerElementMap.end(); ++it)
  {
    std::stringstream elementReport;
    bool isFirst = true;
    for(std::string fileName : it->second)
    {
      std::string pointerStr(" ");
      for(size_t i = it->first.size(); i < maxWidth; i++)
        pointerStr.append("-");
      pointerStr.append("> ");
      std::string elemStr("Element " + it->first);
      if(isFirst)
      {
        isFirst = false;
        elementReport << Utils::getTabSpace(2) << elemStr << pointerStr << fileName << "\n";
      }
      else
        elementReport << Utils::getTabSpace(2) << std::string(elemStr.size(), ' ') << pointerStr << fileName << "\n";
      fileGenCount++;
    }
    std::cout << elementReport.str();
  }
  std::stringstream fileGenCountMsg;
  fileGenCountMsg << "\n===== Success: " << fileGenCount << " element files generated =====";
  Utils::Logger::log(fileGenCountMsg.str());
}

void Generator::reportGeneratedElementFile(const std::string& elementName, const std::string& generatedFileName)
{
  auto findIt = m_elementFilesPerElementMap.find(elementName);
  if(m_elementFilesPerElementMap.end() == findIt)
  {
    std::vector<std::string> files;
    m_elementFilesPerElementMap[elementName] = files;
    findIt = m_elementFilesPerElementMap.find(elementName);
  }
  findIt->second.push_back(generatedFileName);
}

