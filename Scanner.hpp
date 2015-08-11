/**
 *  TEBNF Scanner.
 *
 * @author  Jason Young
 * @version 0.1
 * @since   2013-12-26
 */

#ifndef SCANNER_HPP
#define SCANNER_HPP

//System includes:
#include <memory>
#include <string>
#include <vector>

class Tokens;
class Scanner
{
public:
  /** Constructor.
   */
  Scanner() {}
  /** Loads and scans a grammar.
   */
  static void scan(const std::string& grammarText) { get().loadGrammar(grammarText); }
  static Scanner& get() { static Scanner scanner; return scanner; }
  static std::shared_ptr<Tokens> getTokens() { return get().getTokensHelper(); }
private:
  /** Prevent unwanted copying. */
  Scanner(const Scanner&);
  /** Prevent unwanted copying. */
  const Scanner& operator=(const Scanner&);
  void loadGrammar(const std::string& grammarText);
  std::shared_ptr<Tokens> getTokensHelper() const;
};

#endif //SCANNER_HPP
