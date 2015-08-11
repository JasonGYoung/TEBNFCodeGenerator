/**
 *  Main entry point for the TEBNF Code Generator.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2013-12-26
 */

//Local includes:
#include "Generator.hpp"
#include "Parser/Parser.hpp"
#include "Reader.hpp"
#include "Scanner.hpp"
#include "Utils/Utils.hpp"

//System includes:
#include <cerrno>
#include <iostream>
#include <string>
#include <Windows.h>

int main(int argc, const char* argv[])
{
  std::string exeName("TEBNFCodeGenerator");
  if(argc < 4)
  {
    if(1 == argc)
    {
      std::string strPath(argv[0]);
      auto pos = strPath.find_last_of("/\\");
      exeName = std::string::npos != pos ? strPath.substr(pos + 1) : strPath;
    }
    std::cout << "Usage: " << exeName << " <source> <destination> <name>\n"
      << "Arguments:\n"
      << "  source - Path of file containing TEBNF grammar, including file name.\n"
      << "  destination - Path of the location on disk to write generated files.\n"
      << "  name - Name to give to the generated application.\n";
  }
  else
  {
    try
    {
      std::string srcFilePath(argv[1]);
      std::string destDirPath(argv[2]);
      std::string appName(argv[3]);
      Utils::Logger::setup(srcFilePath); //Setup static logger.
      Utils::Logger::log("\n--------- TEBNF Code Generator v" + Utils::getTEBNFVersion() + " ---------\n");
      Reader::read(srcFilePath);
      Scanner::scan(Reader::getFileText());
      Parser::parse();
      Generator::generate(destDirPath, appName);
      return EXIT_SUCCESS;
    }
    catch(std::exception& ex)
    {
      std::cout << std::string(ex.what()) << std::endl;
    }
  }
  return EXIT_FAILURE;
}
