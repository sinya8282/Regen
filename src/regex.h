#ifndef REGEN_REGEX_H_
#define REGEN_REGEX_H_

#include "util.h"
#include "expr.h"
#include "exprutil.h"
#include "dfa.h"

namespace regen {

class Regex {
public:
  Regex(const std::string &regex);
  ~Regex() { delete expr_root_; };
  void PrintRegex();
  void PrintExtendedRegex();
  void PrintParseTree();
  void DumpExprTree();
  DFA* CreateDFA();
  const std::string& regex() { return regex_; }
  Must& must() { return must_; }
  std::size_t max_lenlgth() { return expr_root_->max_length(); }
  std::size_t min_lenlgth() { return expr_root_->min_length(); }
  std::size_t must_max_length() { return must_max_length_; }
  const std::string& must_max_word() { return must_max_word_; }
private:
  Expr::Type lex();
  Expr* e0();
  Expr* e1();
  Expr* e2();
  Expr* e3();
  void parse();
  std::vector<bool>* create_tbl();

  const std::string regex_;
  Expr *expr_root_;

  std::vector<StateExpr*> states_;
  Must must_;
  std::size_t must_max_length_;
  const std::string must_max_word_;
  std::vector<bool> involved_char_;
  std::size_t count_involved_char_;

  Expr::Type token_type_;
  const char *parse_ptr_;
  char parse_lit_;
  std::size_t expr_id_;
  std::size_t state_id_;

  DFA *dfa_;
};

} // namespace regen

#endif // REGEN_REGEX_H_
