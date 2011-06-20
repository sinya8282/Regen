#ifndef REGEN_PARALLEL_DFA_H_
#define  REGEN_PARALLEL_DFA_H_
#include "util.h"
#include "dfa.h"

namespace regen {

class ParallelDFA: public DFA {
public:
  ParallelDFA(const DFA &dfa, std::size_t thread_num = 2);
  typedef std::map<int, int> ParallelTransition;
  bool Compile();
  bool FullMatch(const std::string &str) const { return FullMatch((unsigned char*)str.c_str(), (unsigned char *)str.c_str()+str.length()); }
  bool FullMatch(const unsigned char *str, const unsigned char *end) const;
  struct TaskArg {
    const unsigned char *str;
    const unsigned char *end;
    std::size_t task_id;
  };
private:
  void FullMatchTask(TaskArg targ) const;
  int *transition__;
  mutable std::vector<int> parallel_states_;
  std::size_t dfa_size_;
  std::size_t thread_num_;
  std::deque<bool> dfa_accepts_;
  std::vector<ParallelTransition> parallel_transitions_;
  int (*CompiledFullMatchTask)(const unsigned char *, const unsigned char *);
};

} // namespace regen
#endif // REGEN_PARALLEL_DFA_H_
