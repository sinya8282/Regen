#ifndef REGEN_REGEX_H_
#define REGEN_REGEX_H_

#include "regen.h"
#include "util.h"
#include "lexer.h"
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
  Regex(const std::string &regex, const Regen::Options = Regen::Options::NoParseFlags);
  ~Regex() { delete expr_root_; };
  void PrintRegex();
  void PrintExtendedRegex() const;
  void PrintParseTree() const;
  Expr* CreateRegexFromDFA(DFA &dfa);
  void DumpExprTree() const;
  bool Compile(Regen::Options::CompileFlag olevel = Regen::Options::O3);
  bool MinimizeDFA() { if (dfa_.Complete()) { dfa_.Minimize(); return true; } else return false; }
  bool Match(const std::string &string, Regen::Context *context = NULL) const;
  bool Match(const char *begin, const char *end, Regen::Context *context = NULL) const;
  bool Match(const unsigned char *begin, const unsigned char *end, Regen::Context *context = NULL) const;
  bool MatchNFA(const unsigned char *begin, const unsigned char *end, Regen::Context *context = NULL) const;
  const std::string& regex() const { return regex_; }
  const Must& must() const { return must_; }
  std::size_t max_lenlgth() const { return expr_root_->max_length(); }
  std::size_t min_lenlgth() const { return expr_root_->min_length(); }
  std::size_t must_max_length() const { return must_max_length_; }
  const std::string& must_max_word() const { return must_max_word_; }
  const DFA& dfa() const { return dfa_; }
  Regen::Options::CompileFlag olevel() const { return olevel_; }
  Expr* expr_root() const { return expr_root_; }
  const std::vector<StateExpr*> &state_exprs() const { return state_exprs_; }

private:
  Expr* Parse(Lexer *);
  Expr* e0(Lexer *);
  Expr* e1(Lexer *);
  Expr* e2(Lexer *);
  Expr* e3(Lexer *);
  Expr* e4(Lexer *);
  Expr* e5(Lexer *);
  Expr* e6(Lexer *);
  CharClass* BuildCharClass(Lexer *);
  StateExpr* CombineStateExpr(StateExpr* e1, StateExpr* e2);
  Expr* PatchBackRef(Lexer *, Expr *);

  const std::string regex_;
  Regen::Options flag_;
  Expr *expr_root_;
  std::size_t recursive_depth_;
  std::vector<StateExpr*> state_exprs_;

  Must must_;
  std::size_t must_max_length_;
  const std::string must_max_word_;
  std::bitset<256> involved_char_;
  std::size_t count_involved_char_;

  std::size_t expr_id_;
  std::size_t state_id_;

  Regen::Options::CompileFlag olevel_;
  bool dfa_failure_;
  DFA dfa_;
};

} // namespace regen

#endif // REGEN_REGEX_H_
