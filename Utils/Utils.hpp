/**
 *  TEBNF utility types and functions.
 *
 * @author  Jason Young
 */

#ifndef TEBNFUTILS_HPP
#define TEBNFUTILS_HPP

//Local includes:
#include "Optional.hpp"
#include "../Token.hpp"

//System includes:
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct CppTypeInfo;
class Element;

namespace Utils
{

  static std::string getTEBNFVersion() { return std::string("1.0.0"); }

  template<typename T>
  T getItemAt(const std::vector<T>& container,
              size_t index,
              const std::string& errorMessage = "")
  {
    if(container.empty() || index >= container.size())
    {
      std::ostringstream err;
      if(errorMessage.empty())
        err << "Container index out of range: " << index;
      else
        err << errorMessage;
      throw std::runtime_error(err.str());
    }
    return container.at(index);
  }

  static const size_t TAB_SIZE = 2;
  static std::string getTabSpace(size_t tabCount = 1, size_t tabSize = TAB_SIZE)
  {
    return std::string((tabCount * tabSize), ' ');
  }

  std::string getDateTimeAsString();

  std::string getClassPrefaceComment(const std::string& fileName,
                                     const std::string& description = std::string());

  void trim(std::string& rStr);

  std::string trimCopy(const std::string& str);

  void toLower(std::string& rStr);

  std::string toLowerCopy(const std::string& str);

  void toUpper(std::string& rStr);

  std::string toUpperCopy(const std::string& str);

  std::string getCppVarName(const std::string& varName);
  std::string getCppVarName(std::shared_ptr<Token> pToken, bool typeQualify = true);
  std::string getCppVarName(const CppTypeInfo& cppInfo, bool typeQualify = true);
  std::string getCppVarName(std::shared_ptr<CppTypeInfo> pCppInfo, bool typeQualify = true);

  std::string getElementAccessor(const CppTypeInfo& cppInfo);
  std::string getElementAccessor(std::shared_ptr<Element> pElement);

  void resetCppVarNameAccessedElementNames();
  std::vector<std::string> getCppVarNameAccessedElementNames(bool reset = true);

  template<typename T>
  typename std::vector<std::shared_ptr<T> >::iterator
    findNodeIf(typename std::vector<std::shared_ptr<T> >& v, std::shared_ptr<T> pNodeToFind)
  {
    return std::find_if(v.begin(), v.end(),
      [&](std::shared_ptr<T> pChild)->bool
      { return pChild->getToken()->text == pNodeToFind->getToken()->text; });
  }

  template<typename T>
  typename std::vector<std::shared_ptr<T> >::iterator
    findNodeIf(typename std::vector<std::shared_ptr<T> >& v, T* pNodeToFind)
  {
    return std::find_if(v.begin(), v.end(),
      [&](std::shared_ptr<T> pChild)->bool
      { return pChild->getToken()->text == pNodeToFind->getToken()->text; });
  }

  float stof(const std::string& str, float defaultVal = 0.);
  double stod(const std::string& str, double defaultVal = 0.);
  long double stold(const std::string& str, long double defaultVal = 0.);

  int stoi(const std::string& str, int base = 10, int defaultVal = 0);
  long stol(const std::string& str, int base = 10, long defaultVal = 0);
  long long stoll(const std::string& str, int base = 10, long long defaultVal = 0);

  class Logger
  {
  public:
    struct EmptyExInfo {};
    /** Message severity level. */
    enum MsgLvl_e { MSG_INFO, MSG_WARN, MSG_ERR };
    /** Constructor.
     */
    Logger() : m_srcFilePath(), m_currentMsg(), m_appendMsgMode(false) {}

    static void appendMessages(bool appendMsgs) { get().appendMessagesHelper(appendMsgs); }

    /** Log functions.
     * @param[in] msgLvl - message severity level.
     * @param[in] lineNum - line number to include in the message.
     * @param[in] pTok - token containing the line number to include in message.
     * @param[in] msg - log message to report.
     * @see comments for logHelper().
     */
    static void log(MsgLvl_e msgLvl, size_t lineNum, const std::string& msg) { get().logHelper(msgLvl, msg, lineNum); }
    static void log(MsgLvl_e msgLvl, std::shared_ptr<Token> pTok, const std::string& msg) { get().logHelper(msgLvl, msg, pTok->lineNumber); }
    static void log(MsgLvl_e msgLvl, const std::string& msg) { get().logHelper(msgLvl, msg); }
    static void log(const std::string& msg, bool enableEndl = true)
    { 
      std::cout << msg;
      if(enableEndl) std::cout << std::endl;
    }

    /** Log functions.
     * @param[in] lineNum - line number to include in the message.
     * @param[in] pTok - token containing the line number to include in message.
     * @param[in] msg - log message to report.
     * @see comments for logHelper().
     */
    static void logInfo(size_t lineNum, const std::string& msg) { get().logHelper(MSG_INFO, msg, lineNum); }
    static void logWarn(size_t lineNum, const std::string& msg) { get().logHelper(MSG_WARN, msg, lineNum); }
    static void logErr(size_t lineNum, const std::string& msg) { get().logHelper(MSG_ERR, msg, lineNum); }
    static void logInfo(std::shared_ptr<Token> pTok, const std::string& msg) { get().logHelper(MSG_INFO, "\"" + pTok->text + "\": " + msg, pTok->lineNumber); }
    static void logWarn(std::shared_ptr<Token> pTok, const std::string& msg) { get().logHelper(MSG_WARN, "\"" + pTok->text + "\": " + msg, pTok->lineNumber); }
    static void logErr(std::shared_ptr<Token> pTok, const std::string& msg) { get().logHelper(MSG_ERR, "\"" + pTok->text + "\": " + msg, pTok->lineNumber); }
    static void logInfo(const std::string& msg) { get().logHelper(MSG_INFO, msg); }
    static void logWarn(const std::string& msg) { get().logHelper(MSG_WARN, msg); }
    static void logErr(const std::string& msg) { get().logHelper(MSG_ERR, msg); }

    static void logAppendedMessages() { get().logAppendedMessagesHelper(true); }

    /** Get a static instance of the logger.
     * @param[in] srcFilePath - source file path to use in log messages.
     * @return the static instance of this logger.
     */
    static Logger& get(const std::string& srcFilePath = "")
    {
      static Logger logger;
      if(!srcFilePath.empty())
        logger.setSrcFilePath(srcFilePath);
      return logger;
    }

    /** Setup the logger.
     * @param[in] srcFilePath - source file path to use in log messages.
     */
    static void setup(const std::string& srcFilePath) { get(srcFilePath); }

    /** Set the source file path.  NOT intended to be called directly.
     * @param[in] srcFilePath - source file path.
     */
    void setSrcFilePath(const std::string& srcFilePath) { m_srcFilePath = srcFilePath; }

    void appendMessagesHelper(bool appendMsgs) { m_appendMsgMode = appendMsgs; }

    void logAppendedMessagesHelper(bool exit)
    {
      m_appendMsgMode = false;
      std::string currentMsg(m_currentMsg.str());
      if(currentMsg.empty()) //If there's nothing to report, do nothing and return.
        return;
      m_currentMsg.str(""); //Clear contents of stream.
      m_currentMsg.clear(); //Clear stream error flags.
      if(exit)
        throw std::runtime_error(currentMsg);
      else
        std::cout << currentMsg << std::endl;
    }

    /** Reports a log message.  NOT intended to be called directly.
     * @param[in] msgLvl - message severity level.
     * @param[in] lineNum - line number to include in the message.
     * @param[in] msg - log message to report.
     * @throws TEBNF_base_exception.
     */
    void logHelper(MsgLvl_e msgLvl,
                   const std::string& msg,
                   optional<size_t> optLineNum = optional<size_t>())
    {
      m_currentMsg << m_srcFilePath;
      if(optLineNum)
        m_currentMsg << " (" << *optLineNum << "): ";
      bool exit = false;
      switch(msgLvl)
      {
      case MSG_INFO: m_currentMsg << "information: "; break;
      case MSG_WARN: m_currentMsg << "warning: "; break;
      case MSG_ERR: m_currentMsg << "error: "; exit = true; break;
      };
      m_currentMsg << msg;
      if(!m_appendMsgMode)
        logAppendedMessagesHelper(exit);
      else
        m_currentMsg << std::endl;
    }

  private:
    std::string m_srcFilePath;
    std::ostringstream m_currentMsg;
    bool m_appendMsgMode;
  };

} //namespace Utils

#endif //TEBNFUTILS_HPP
