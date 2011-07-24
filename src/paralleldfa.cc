#ifdef __ENABLE_PARALLEL__
#include "paralleldfa.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace regen {

ParallelDFA::ParallelDFA(const DFA &dfa, std::size_t thread_num):
    dfa_size_(dfa.size()),
    thread_num_(thread_num)
{
  dfa_accepts_ = dfa.accepts();

  ParallelTransition parallel_transition;
  std::map<ParallelTransition, int> pdfa_map;
  std::queue<ParallelTransition> queue;
  ParallelTransition::iterator iter;
  std::size_t pdfa_id = 0;

  pdfa_map[parallel_transition] = DFA::REJECT;

  for (std::size_t i = 0; i < dfa_size_; i++) {
    parallel_transition[i] = i;
  }

  pdfa_map[parallel_transition] = pdfa_id++;
  queue.push(parallel_transition);

  while (!queue.empty()) {
    parallel_transition = queue.front();
    parallel_transitions_.push_back(parallel_transition);
    queue.pop();
    std::vector<ParallelTransition> transition(256);
    bool is_accept = parallel_transition.size() == dfa_size_;

     iter = parallel_transition.begin();
      while (iter != parallel_transition.end()) {
        int start = (*iter).first;
        int current = (*iter).second;
        const Transition &trans = dfa.GetTransition(current);
        for (std::size_t c = 0; c < 256; c++) {
          int next = trans[c];
          if (next != DFA::REJECT) {
            transition[c][start] = next;
          }
        }
        if (!dfa_accepts_[current]) is_accept = false;
        ++iter;
     }

    DFA::Transition &dfa_transition = get_new_transition();
    std::set<int> dst_state;
    bool has_reject = false;
    
    for (std::size_t c = 0; c < 256; c++) {
      ParallelTransition &next = transition[c];
      if (next.empty()) {
        has_reject = true;
        continue;
      }

      if (pdfa_map.find(next) == pdfa_map.end()) {
        pdfa_map[next] = pdfa_id++;
        queue.push(next);
      }
      dfa_transition[c] = pdfa_map[next];
      dst_state.insert(pdfa_map[next]);
    }
    if (has_reject) dst_state.insert(DFA::REJECT);
    set_state_info(is_accept ,DFA::REJECT, dst_state);
  }
}

void
ParallelDFA::FullMatchTask(TaskArg targ) const
{
  const unsigned char *str = targ.str;
  const unsigned char *end = targ.end;

  if (compiled_) {
    parallel_states_[targ.task_id] = CompiledFullMatch(str, end);
    return;
  }
  
  int state = 0;

  while (str != end && (state = transition_[state][*str++]) != DFA::REJECT);

  parallel_states_[targ.task_id] = state;
  return;
}

bool
ParallelDFA::FullMatch(const unsigned char *str, const unsigned char *end) const
{
  parallel_states_.resize(thread_num_);
  boost::thread *threads[thread_num_];
  std::size_t task_string_length = (end-str) / thread_num_;
  std::size_t remainder_length = (end-str) % thread_num_;
  const unsigned char *str_ = str, *end_;
  TaskArg targ;

  for (std::size_t i = 0; i < thread_num_; i++) {
    end_ = str_ + task_string_length;
    if (i == thread_num_ - 1) end_ += remainder_length;
    targ.str = str_;
    targ.end = end_;
    targ.task_id = i;
    threads[i] = new boost::thread(
        boost::bind(
            boost::bind(&regen::ParallelDFA::FullMatchTask, this, _1),
            targ));
    str_ = end_;
  }

  int dstate = 0, pstate;
  bool match = false;
  for (std::size_t i = 0; i < thread_num_; i++) {
    threads[i]->join();
    if ((pstate = parallel_states_[i]) == DFA::REJECT) {
      dstate = DFA::REJECT;
      break;
    }
    ParallelTransition::const_iterator iter = parallel_transitions_[pstate].find(dstate);
    if (iter == parallel_transitions_[pstate].end()) break;
    dstate = (*iter).second;
  }

  if (dstate != DFA::REJECT) {
    match = dfa_accepts_[dstate];
  }

  for (std::size_t i = 0; i < thread_num_; i++) {
    threads[i]->join();
    delete threads[i];
  }
  
  return match;
}

} // namespace regen

#endif //__ENABLE_PARALLEL__
