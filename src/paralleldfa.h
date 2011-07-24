#ifdef __ENABLE_PARALLEL__
#ifndef REGEN_PARALLEL_DFA_H_
#define  REGEN_PARALLEL_DFA_H_
#include "util.h"
#include "dfa.h"

namespace regen {

class ParallelDFA: public DFA {
public:
  ParallelDFA(const DFA &dfa, std::size_t thread_num = 2);
  std::size_t thread_num() const { return thread_num_; }
  void thread_num(std::size_t thread_num) { thread_num_ = thread_num; }
  typedef std::map<int, int> ParallelTransition;
  void Minimize();
  bool FullMatch(const unsigned char *str, const unsigned char *end) const;
  struct TaskArg {
    const unsigned char *str;
    const unsigned char *end;
    std::size_t task_id;
  };
private:
  void FullMatchTask(TaskArg targ) const;
  mutable std::vector<int> parallel_states_;
  std::size_t dfa_size_;
  std::size_t thread_num_;
  std::deque<bool> dfa_accepts_;
  std::vector<ParallelTransition> parallel_transitions_;
};

} // namespace regen
#endif // REGEN_PARALLEL_DFA_H_
#endif // __ENABLE_PARALLEL__
