#ifndef REGEN_SSFA_H_
#define  REGEN_SSFA_H_
#ifdef REGEN_ENABLE_PARALLEL
#include "regen.h"
#include "regex.h"
#include "util.h"
#include "expr.h"
#include "nfa.h"
#include "dfa.h"

class Regex;

namespace regen {

class SSFA: public DFA {
public:
  SSFA(Expr* expr_root, const std::vector<StateExpr*> &state_exprs, std::size_t thread_num = 2);
  SSFA(const NFA &nfa, std::size_t thread_num = 2);  
  SSFA(const DFA &dfa, std::size_t thread_num = 2);
  std::size_t thread_num() const { return thread_num_; }
  void thread_num(std::size_t thread_num) { thread_num_ = thread_num; }
  typedef std::map<state_t, std::set<state_t> > SSTransition;
  typedef std::map<state_t, state_t> SSDTransition;
  bool Minimize() { return true; }
  bool Match(const std::string &str) const { return Match((unsigned char*)str.c_str(), (unsigned char *)str.c_str()+str.length()); }
  bool Match(const unsigned char *str, const unsigned char *end) const;
  struct TaskArg {
    const unsigned char *str;
    const unsigned char *end;
    std::size_t task_id;
  };
private:
  void MatchTask(TaskArg targ) const;
  mutable std::vector<state_t> partial_results_;
  std::size_t nfa_size_;
  std::size_t dfa_size_;
  std::set<state_t> start_states_;
  std::size_t thread_num_;
  std::vector<bool> fa_accepts_;
  std::vector<SSTransition> sst_;
};

} // namespace regen
#else
#include "expr.h"
#include "dfa.h"
namespace regen {
class SSFA: public DFA {
 public:
    SSFA(Expr* expr_root, const std::vector<StateExpr*> &state_exprs, std::size_t thread_num = 2) { exitmsg("SSFA not available. (recompile with REGEN_ENABLE_PARALLEL=yes)"); }
  SSFA(const NFA &nfa, std::size_t thread_num = 2) { exitmsg("SSFA not available. (recompile with REGEN_ENABLE_PARALLEL=yes)"); }
  SSFA(const DFA &dfa, std::size_t thread_num = 2) { exitmsg("SSFA not available. (recompile with REGEN_ENABLE_PARALLEL=yes)"); }
};
}
#endif
#endif // REGEN_SSFA_H_
