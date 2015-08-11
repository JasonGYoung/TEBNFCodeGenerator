/**
 *  TEBNF IO Element type.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2014-04-26
 */

#ifndef IOELEMENT_HPP
#define IOELEMENT_HPP

#include "Element.hpp"

struct SupportClass;

class IoElement : public Element
{
public:
  IoElement(const std::shared_ptr<Token>& pTok) : Element(pTok) {}
  virtual std::string getTypeName() const { return "IoElement"; }
  virtual std::string getMemberAccessor() const { return "::get()->"; }
  virtual void generateCode();

  static std::string generateConsoleIoHpp(const std::string& typeName, const IoElement* pIoElem);

  static std::shared_ptr<SupportClass> generateWsaSessionHpp();
  static std::string generateUdpSocketHpp(const std::string& typeName);
  static std::string generateUdpSocketCpp(const std::string& typeName);

  static std::string generateFileIoHpp(const std::string& typeName);
  static std::string generateFileIoCpp(const std::string& typeName);
};

#endif //IOELEMENT_HPP