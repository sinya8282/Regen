#ifndef REGEN_REGEX_H_
#define REGEN_REGEX_H_

#include "regen.h"
#include "util.h"
#include "lexer.h"
#include "expr.h"
#include "exprutil.h"
#include "generator.h"
#include "nfa.h"
#include "dfa.h"
#ifdef REGEN_ENABLE_PARALLEL
#include "ssfa.h"
#endif

namespace regen {

class Regex {
public:
  Regex(const std::string &regex, const Regen::Options = Regen::Options::NoParseFlags);
  ~Regex() {}
  void PrintRegex() const;
  static void PrintRegex(const DFA &);
  void PrintParseTree() const;
  void PrintText(Expr::GenOpt, std::size_t n = 1) const;
  static void CreateRegexFromDFA(const DFA &dfa, ExprInfo *info, ExprPool *p);
  void DumpExprTree() const;
  bool Compile(Regen::Options::CompileFlag olevel = Regen::Options::O3);
  bool MinimizeDFA() { if (dfa_.Complete()) { dfa_.Minimize(); return true; } else return false; }
  bool Match(const std::string &string, Regen::Context *context = NULL) const;
  bool Match(const char *begin, const char *end, Regen::Context *context = NULL) const;
  bool Match(const unsigned char *begin, const unsigned char *end, Regen::Context *context = NULL) const;
  bool NFAMatch(const unsigned char *begin, const unsigned char *end, Regen::Context *context = NULL) const;
  const std::string& regex() const { return regex_; }
  const Must& must() const { return must_; }
  std::size_t max_lenlgth() const { return expr_info_.expr_root->max_length(); }
  std::size_t min_lenlgth() const { return expr_info_.expr_root->min_length(); }
  std::size_t must_max_length() const { return must_max_length_; }
  const std::string& must_max_word() const { return must_max_word_; }
  const DFA& dfa() const { return dfa_; }
  Regen::Options::CompileFlag olevel() const { return olevel_; }
  Expr* expr_root() const { return expr_info_.expr_root; }
  const ExprInfo& expr_info() const { return expr_info_; }
  const std::vector<StateExpr*> &state_exprs() const { return state_exprs_; }
  static CharClass* BuildCharClass(Lexer *, CharClass *);

private:
  void Parse();
  Expr* e0(Lexer *, ExprPool *);
  Expr* e1(Lexer *, ExprPool *);
  Expr* e2(Lexer *, ExprPool *);
  Expr* e3(Lexer *, ExprPool *);
  Expr* e4(Lexer *, ExprPool *);
  Expr* e5(Lexer *, ExprPool *);
  Expr* e6(Lexer *, ExprPool *);
  static StateExpr* CombineStateExpr(StateExpr*, StateExpr*, ExprPool *);
  Expr* PatchBackRef(Lexer *, Expr *, ExprPool *);

  const std::string regex_;
  Regen::Options flag_;
  ExprInfo expr_info_;
  ExprPool pool_;
  std::size_t recursion_depth_;
  std::vector<StateExpr*> state_exprs_;

  Must must_;
  std::size_t must_max_length_;
  const std::string must_max_word_;
  std::bitset<256> involved_char_;
  std::size_t count_involved_char_;

  Regen::Options::CompileFlag olevel_;
  bool dfa_failure_;
  DFA dfa_;
};

} // namespace regen

#endif // REGEN_REGEX_H_
