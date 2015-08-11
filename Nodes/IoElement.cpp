
//Primary include:
#include "IoElement.hpp"

#include "Elements.hpp"

void IoElement::generateCode()
{
  if(!getCppTypeInfo())
    setCppTypeInfo(std::make_shared<CppTypeInfo>());
  std::string typeName(getCppTypeInfo()->typeNameStr);

  if(isAsElement())
  {
    return; //TRICKY: we'll generate a typedef in the state table element code.
  }

  getCppTypeInfo()->cppIncludes.push_back("#include \"" + typeName + ".hpp\"\n");
  auto elemType = getElementType();
  switch(elemType)
  {
  case Types::ELEMENT_TYPE_IO_INPUT_UDP_IP: //Fall-through since both input and output use same socket class.
  case Types::ELEMENT_TYPE_IO_OUTPUT_UDP_IP:
    getCppTypeInfo()->hppStatements.push_back(generateUdpSocketHpp(typeName));
    getCppTypeInfo()->cppStatements.push_back(generateUdpSocketCpp(typeName));
    getCppTypeInfo()->supportClasses.push_back(generateWsaSessionHpp());
    break;
  case Types::ELEMENT_TYPE_IO_INPUT_CONSOLE:
  case Types::ELEMENT_TYPE_IO_OUTPUT_CONSOLE:
    getCppTypeInfo()->hppStatements.push_back(generateConsoleIoHpp(typeName, this));
    break;
  case Types::ELEMENT_TYPE_IO_INPUT_FILE:
  case Types::ELEMENT_TYPE_IO_OUTPUT_FILE:
    getCppTypeInfo()->hppStatements.push_back(generateFileIoHpp(typeName));
    getCppTypeInfo()->cppStatements.push_back(generateFileIoCpp(typeName));
    break;
  case Types::ELEMENT_TYPE_IO_INPUT_GUI:
  case Types::ELEMENT_TYPE_IO_OUTPUT_GUI:
  case Types::ELEMENT_TYPE_IO_INPUT_MYSQL:
  case Types::ELEMENT_TYPE_IO_OUTPUT_MYSQL:
  case Types::ELEMENT_TYPE_IO_INPUT_TCP_IP:
  case Types::ELEMENT_TYPE_IO_OUTPUT_TCP_IP:
    //TODO
    break;
  default:
    Utils::Logger::logErr(getToken(), "Unsupported IO element type");
    break;
  };
  //Append includes in cpp for element classes accessed by this generated class.
  getCppTypeInfo()->appendAccessedElementIncludes();
}

std::string IoElement::generateConsoleIoHpp(const std::string& typeName, const IoElement* pIoElem)
{
  std::string spc = Utils::getTabSpace();
  std::string code(
    "#include \"Conversion.hpp\"\n\n"

    "#include <cstdint>\n"
    "#include <iomanip>\n"
    "#include <iostream>\n"
    "#include <memory>\n"
    "#include <mutex>\n"
    "#include <string>\n"
    "#include <vector>\n\n"

    "class " + typeName + "\n"
    "{\n"
    "public:\n"
    + spc + "std::vector<uint8_t> m_data;\n"
    + spc + "size_t m_offset;\n"
    + spc + "std::mutex m_mutex;\n\n"

    + spc + typeName + "() : m_data(), m_offset(0) {}\n"

    + spc + "void write(const std::vector<uint8_t>& buffer, size_t offset, size_t len)\n"
    + spc + "{\n"
    + spc + spc + "std::cout << std::setw(len) << &buffer[offset] << std::endl;\n"
    + spc + "}\n\n"

    + spc + "template<typename T>\n"
    + spc + "void write(const T& val)\n"
    + spc + "{\n"
    + spc + spc + "std::cout << val << std::endl;\n"
    + spc + "}\n\n"

    + spc + "template<typename T>\n"
    + spc + "bool read(const std::string& prompt = std::string())\n"
    + spc + "{\n"
    + spc + spc + "T in;\n"
    + spc + spc + "if(!prompt.empty()) std::cout << prompt;\n"
    + spc + spc + "std::cin >> in;\n"
    + spc + spc + "Conversion::marshal(m_data, in, false);\n"
    + spc + spc + "return true;\n"
    + spc + "}\n\n"

    + spc + "void adjust(bool /* doAdjustment */) {} // Unneeded for console IO.\n\n"

    + spc + "static std::shared_ptr<" + typeName + "> get()\n"
    + spc + "{\n"
    + spc + spc + "std::lock_guard<std::mutex> lock(m_mutex);\n"
    + spc + spc + "static auto pInstance = std::make_shared<" + typeName + ">();\n"
    + spc + spc + "return pInstance;\n"
    + spc + "}\n"
    "};\n");
  return code;
}

std::shared_ptr<SupportClass> IoElement::generateWsaSessionHpp()
{
  std::string spc = Utils::getTabSpace();
  auto pSupCls = std::make_shared<SupportClass>("WSASession");
  pSupCls->hppIncludes.push_back(
    "#ifndef WSASESSION_HPP\n"
    "#define WSASESSION_HPP\n\n"

    "#include <Winsock2.h>\n\n");

  pSupCls->hppStatements.push_back("class WSASession\n"
    "{\n"
    "public:\n"
    + spc + "WSASession()"
    + spc + "{\n"
    + spc + spc + "int ret = WSAStartup(MAKEWORD(2, 2), &m_data);\n"
    + spc + spc + "if(ret != 0)\n"
    + spc + spc + spc + "throw std::system_error(WSAGetLastError(), std::system_category(), \"WSAStartup Failed\");\n"
    + spc + "}\n\n"
    + spc + "~WSASession() { WSACleanup(); }\n"
    "private:\n"
    + spc + "WSAData m_data;\n"
    "};\n\n"

    "#endif //WSASESSION_HPP\n\n");

  return pSupCls;
}

//static
std::string IoElement::generateUdpSocketHpp(const std::string& typeName)
{
  std::string spc = Utils::getTabSpace();
  return std::string(
    "#include <WinSock2.h>\n"
    "#include <WS2tcpip.h>\n"
    "#pragma comment(lib, \"Ws2_32.lib\")\n"
    "#include <memory>\n"
    "#include <mutex>\n"
    "#include <string>\n"
    "#include <vector>\n"
    "#include \"WSASession.hpp\"\n"

    "class " + typeName + "\n"
    "{\n"
    "public:\n"
    + spc + "std::vector<uint8_t> m_data;\n"
    + spc + "size_t m_offset;\n"
    + spc + "std::mutex m_mutex;\n"

    + spc + typeName + "();\n"
    + spc + typeName + "(unsigned short port, const std::string& address = std::string(""));\n"
    + spc + "~" + typeName + "();\n"
    + spc + "void write(const uint8_t* pBuffer, size_t len);\n"
    + spc + "bool read(uint8_t* pBuffer, size_t len);\n"
    + spc + "bool read();\n"
    + spc + "void adjust(bool doAdjustment);\n"
    + spc + "int getMtuSize();\n"
    + spc + "static std::shared_ptr<" + typeName + "> get()\n"
    + spc + "{\n"
    + spc + spc + "std::lock_guard<std::mutex> lock(m_mutex);\n"
    + spc + spc + "static auto pInstance = std::make_shared<" + typeName + ">();\n"
    + spc + spc + "return pInstance;\n"
    + spc + "}\n"
    + spc + "static std::shared_ptr<" + typeName + "> get(unsigned short port, const std::string& address = std::string(""))\n"
    + spc + "{\n"
    + spc + spc + "static auto pInstance = std::make_shared<" + typeName + ">(port, address);\n"
    + spc + spc + "return pInstance;\n"
    + spc + "}\n"
    "private:\n"
    + spc + "std::shared_ptr<sockaddr_in> m_pSockAddressIn;\n"
    + spc + "SOCKET m_sock;\n"
    + spc + "std::string m_address;\n"
    + spc + "unsigned short m_port;\n"
    + spc + "int m_mtuMaxSize;\n"
    + spc + "size_t m_readLen;\n"
    "};\n\n");
}

//static
std::string IoElement::generateUdpSocketCpp(const std::string& typeName)
{
  std::string spc = Utils::getTabSpace();
  return std::string("//System includes:\n"
    "#include <system_error>\n"
    "#include <iostream>\n\n"

    + typeName + "::" + typeName + "()\n"
    ": m_data(),\n"
    + spc + "m_offset(0),\n"
    + spc + "m_pSockAddressIn(std::make_shared<sockaddr_in>()),\n"
    + spc + "m_sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)),\n"
    + spc + "m_address(),\n"
    + spc + "m_port(),\n"
    + spc + "m_mtuMaxSize(-1),\n"
    + spc + "m_readLen(0)\n"
    "{\n"
    + spc + "std::cout << \"IP address: \";\n"
    + spc + "std::cin >> m_address;\n"
    + spc + "std::cout << \"Port number: \";\n"
    + spc + "std::cin >> m_port;\n"
    + spc + "std::cout << std::endl;\n"
    "}\n\n"
    + typeName + "::" + typeName + "(unsigned short port, const std::string& address)\n"
    ": m_data(),\n"
    + spc + "m_offset(0),\n"
    + spc + "m_pSockAddressIn(std::make_shared<sockaddr_in>()),\n"
    + spc + "m_sock(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)),\n"
    + spc + "m_address(address),\n"
    + spc + "m_port(port),\n"
    + spc + "m_mtuMaxSize(-1),\n"
    + spc + "m_readLen(0)\n"
    "{\n"
    + spc + "if(m_sock == INVALID_SOCKET)\n"
    + spc + spc + "throw std::system_error(WSAGetLastError(), std::system_category(), \"Error opening socket\");\n"
    + spc + "if(m_address.empty())\n"
    + spc + "{\n"
    + spc + spc + "sockaddr_in add;\n"
    + spc + spc + "add.sin_family = AF_INET;\n"
    + spc + spc + "add.sin_addr.s_addr = htonl(INADDR_ANY);\n"
    + spc + spc + "add.sin_port = htons(m_port);\n"
    + spc + spc + "if(bind(m_sock, reinterpret_cast<SOCKADDR*>(&add), sizeof(add)) < 0)\n"
    + spc + spc + "throw std::system_error(WSAGetLastError(), std::system_category(), \"Bind failed\");\n"
    + spc + "}\n"
    + spc + "getMtuSize();\n"
    "}\n\n" +

    typeName + "::~" + typeName + "()\n"
    "{\n"
    + spc + "closesocket(m_sock);\n"
    "}\n\n"

    "void " + typeName + "::write(const uint8_t* pBuffer, size_t len)\n"
    "{\n"
    + spc + "SOCKADDR* pSockAddr = NULL;\n"
    + spc + "if(m_address.empty())\n"
    + spc + spc + "pSockAddr = reinterpret_cast<SOCKADDR*>(m_pSockAddressIn.get());\n"
    + spc + "else\n"
    + spc + "{\n"
    + spc + spc + "m_pSockAddressIn->sin_family = AF_INET;\n"
    + spc + spc + "m_pSockAddressIn->sin_port = htons(m_port);\n"
    + spc + spc + "if(1 != InetPton(AF_INET, m_address.c_str(), &(m_pSockAddressIn->sin_addr.s_addr)))\n"
    + spc + spc + spc + "throw std::system_error(WSAGetLastError(), std::system_category(), \"write failed\");\n"
    + spc + spc + "pSockAddr = reinterpret_cast<SOCKADDR *>(m_pSockAddressIn.get());\n"
    + spc + "}\n"
    + spc + "int currentLen = static_cast<int>(len);\n"
    + spc + "while(currentLen > 0)\n"
    + spc + "{\n"
    + spc + spc + "int sendSize = currentLen < getMtuSize() ? currentLen : getMtuSize();\n"
    + spc + spc + "if(sendto(m_sock, reinterpret_cast<const char*>(pBuffer), static_cast<int>(sendSize), 0, pSockAddr, sizeof(*pSockAddr)) < 0)\n"
    + spc + spc + spc + "throw std::system_error(WSAGetLastError(), std::system_category(), \"write failed\");\n"
    + spc + spc + "pBuffer += sendSize;\n"
    + spc + spc + "currentLen -= sendSize;\n"
    + spc + "}\n"
    "}\n\n"

    "bool " + typeName + "::read(uint8_t* pBuffer, size_t len)\n"
    "{\n"
    + spc + "sockaddr_in from;\n"
    + spc + "int size = sizeof(from);\n"
    + spc + "int ret = recvfrom(m_sock, reinterpret_cast<char*>(pBuffer), static_cast<int>(len), 0, reinterpret_cast<SOCKADDR*>(&from), &size);\n"
    + spc + "if(ret < 0) return false;\n"    
    + spc + "m_pSockAddressIn = std::make_shared<sockaddr_in>(from);\n"
    + spc + "return ret > 0;\n"
    "}\n\n"

    "bool " + typeName + "::read()\n"
    "{\n"
    + spc + "m_readLen = getMtuSize();\n"
    + spc + "if(m_data.empty()) m_data.resize(m_readLen);\n"
    + spc + "return read(&m_data[m_offset], m_readLen);\n"
    "}\n\n"

    "void " + typeName + "::adjust(bool doAdjustment)\n"
    "{\n"
    + spc + "if(doAdjustment)\n"
    + spc + "{\n"
    + spc + spc + "m_offset += m_readLen;\n"
    + spc + spc + "m_data.resize(m_data.size() + m_readLen); /*Make room for next read*/\n"
    + spc + "}\n"
    + spc + "else\n"
    + spc + "{\n"
    + spc + spc + "m_offset = 0;\n"
    + spc + "}\n"
    "}\n\n"

    "int " + typeName + "::getMtuSize()\n"
    "{\n"
    + spc + "if(-1 == m_mtuMaxSize)\n"
    + spc + "{\n"
    + spc + "  int iSize = sizeof(m_mtuMaxSize);\n"
    + spc + "  if(SOCKET_ERROR == getsockopt(m_sock, SOL_SOCKET, SO_MAX_MSG_SIZE, reinterpret_cast<char*>(&m_mtuMaxSize), &iSize))\n"
    + spc + "    throw std::system_error(WSAGetLastError(), std::system_category(), \"Error determining maximum MTU size\");\n"
    + spc + "}\n"
    + spc + "return m_mtuMaxSize;\n"
    "}\n");
}

std::string IoElement::generateFileIoHpp(const std::string& typeName)
{
  std::string spc = Utils::getTabSpace();
  return std::string(
    "//System includes:\n"
    "#include <fstream>\n"
    "#include <iostream>\n"
    "#include <memory>\n"
    "#include <mutex>\n"
    "#include <string>\n"
    "#include <vector>\n\n"
    "class " + typeName + "\n"
    "{\n"
    "public:\n"
    + spc + "std::vector<uint8_t> m_data;\n"
    + spc + "size_t m_offset;\n"
    + spc + "std::mutex m_mutex;\n"

    + spc + typeName + "() : m_data(), m_offset(0) {}\n"
    + spc + "void write(const uint8_t* pBuffer, size_t len, const std::string& filePath = std::string(), bool append = false);\n"
    + spc + "bool read(uint8_t* pBuffer, size_t len, const std::string& filePath = std::string());\n"
    + spc + "bool read(const std::string& filePath = std::string());\n"
    + spc + "void adjust(bool /* doAdjustment */) {} // Unneeded for file IO.\n\n"
    + spc + "static std::shared_ptr<" + typeName + "> get()\n"
    + spc + "{\n"
    + spc + spc + "std::lock_guard<std::mutex> lock(m_mutex);\n"
    + spc + spc + "static auto pInstance = std::make_shared<" + typeName + ">();\n"
    + spc + spc + "return pInstance;\n"
    + spc + "}\n"
    "};\n\n");
}

std::string IoElement::generateFileIoCpp(const std::string& typeName)
{
  std::string spc = Utils::getTabSpace();
  return std::string(
    "#include <sys/stat.h>\n\n"
    "void " + typeName + "::write(const uint8_t* pBuffer, size_t len, const std::string& filePath, bool append)\n"
    "{\n"
    + spc + "std::string fPath(filePath);\n"
    + spc + "try\n"
    + spc + "{\n"
    + spc + spc + "if(filePath.empty())\n"
    + spc + spc + "{\n"
    + spc + spc + spc + "std::cout << \"Write file path : \";\n"
    + spc + spc + spc + "std::cin >> fPath;\n"
    + spc + spc + spc + "std::cout << std::endl;\n"
    + spc + spc + "}\n"
    + spc + spc + "std::ofstream ofs(fPath, (append ? std::ofstream::binary | std::ofstream::app : std::ofstream::binary));\n"
    + spc + spc + "ofs.write(reinterpret_cast<const char*>(pBuffer), len);\n"
    + spc + spc + "ofs.flush();\n"
    + spc + spc + "ofs.close();\n"
    + spc + "}\n"
    + spc + "catch(const std::exception& ex)\n"
    + spc + "{\n"
    + spc + spc + "std::cerr << \"Error writing to \" << filePath << \": \" << std::string(ex.what()) << std::endl;\n"
    + spc + "}\n"
    "}\n\n"

    "bool " + typeName + "::read(uint8_t* pBuffer, size_t len, const std::string& filePath)\n"
    "{\n"
    + spc + "std::string fPath(filePath);\n"
    + spc + "try\n"
    + spc + "{\n"
    + spc + spc + "if(filePath.empty())\n"
    + spc + spc + "{\n"
    + spc + spc + spc + "std::cout << \"Read file path : \";\n"
    + spc + spc + spc + "std::cin >> fPath;\n"
    + spc + spc + spc + "std::cout << std::endl;\n"
    + spc + spc + "}\n"
    + spc + spc + "std::ifstream ifs(fPath, std::ifstream::binary);\n"
    + spc + spc + "ifs.read(reinterpret_cast<char*>(pBuffer), len);\n"
    + spc + spc + "ifs.close();\n"
    + spc + spc + "return true;\n"
    + spc + "}\n"
    + spc + "catch(const std::exception& ex)\n"
    + spc + "{\n"
    + spc + spc + "std::cerr << \"Error reading from \" << filePath << \": \" << std::string(ex.what()) << std::endl;\n"
    + spc + "}\n"
    + spc + "return false;\n"
    "}\n\n"
    
    "bool " + typeName + "::read(const std::string& filePath)\n"
    "{\n"
    + spc + "std::string fPath(filePath);\n"
    + spc + "if(fPath.empty())\n"
    + spc + "{\n"
    + spc + spc + "std::cout << \"Read file path : \";\n"
    + spc + spc + "std::cin >> fPath;\n"
    + spc + spc + "std::cout << std::endl;\n"
    + spc + "}\n"
    + spc + "struct stat st;\n"
    + spc + "size_t len = (0 == stat(fPath.c_str(), &st)) ? st.st_size : 0;\n"
    + spc + "if(0 == len) return false;\n"
    + spc + "m_data.resize(len);\n"
    + spc + "return read(&m_data[0], len, fPath);\n"
    "}\n\n");
}
