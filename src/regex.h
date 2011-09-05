#ifndef REGEN_REGEX_H_
#define REGEN_REGEX_H_

#include "util.h"
#include "expr.h"
#include "exprutil.h"
#include "nfa.h"
#include "dfa.h"
#ifdef REGEN_ENABLE_PARALLEL
#include "ssfa.h"
#endif

namespace regen {
  
class Regex {
public:
  Regex(const std::string &regex, std::size_t recursive_depth_ = 2);
  ~Regex() { delete expr_root_; };
  void PrintRegex();
  void PrintExtendedRegex() const;
  void PrintParseTree() const;
  Expr* CreateRegexFromDFA(DFA &dfa);
  void DumpExprTree() const;
  bool Compile(CompileFlag olevel = O3);
  bool MinimizeDFA() { if (dfa_.Complete()) { dfa_.Minimize(); return true; } else return false; }
  bool FullMatch(const std::string &string) const;
  bool FullMatch(const unsigned char *begin, const unsigned char *end) const;
  bool FullMatchNFA(const unsigned char *begin, const unsigned char *end) const;
  const std::string& regex() const { return regex_; }
  const Must& must() const { return must_; }
  std::size_t max_lenlgth() const { return expr_root_->max_length(); }
  std::size_t min_lenlgth() const { return expr_root_->min_length(); }
  std::size_t must_max_length() const { return must_max_length_; }
  const std::string& must_max_word() const { return must_max_word_; }
  const DFA& dfa() const { return dfa_; }
  CompileFlag olevel() const { return olevel_; }
  Expr* expr_root() const { return expr_root_; }
  const std::vector<StateExpr*> &state_exprs() const { return state_exprs_; }  

private:
  Expr::Type lex();
  void lex_metachar();
  void lex_repetition();
  Expr* e0();
  Expr* e1();
  Expr* e2();
  Expr* e3();
  Expr* e4();
  void Parse();
  //bool MakeDFA(Expr* e, DFA &dfa, int limit = -1, std::size_t neop = 1);
  //bool MakeDFA(NFA &nfa, DFA &dfa);
  CharClass* BuildCharClass();
  StateExpr* CombineStateExpr(StateExpr* e1, StateExpr* e2);

  const std::string regex_;
  Expr *expr_root_;
  std::stack<const char *> parse_stack_;
  bool macro_expand_;
  std::size_t recursive_depth_;
  std::size_t recursive_limit_;
  std::vector<StateExpr*> state_exprs_;

  Must must_;
  std::size_t must_max_length_;
  const std::string must_max_word_;
  std::bitset<256> involved_char_;
  std::size_t count_involved_char_;

  Expr::Type token_type_;
  const char *parse_ptr_;
  unsigned char parse_lit_;
  int lower_repetition_, upper_repetition_;
  std::size_t expr_id_;
  std::size_t state_id_;

  CompileFlag olevel_;
  bool dfa_failure_;
  DFA dfa_;
};

} // namespace regen

#endif // REGEN_REGEX_H_
