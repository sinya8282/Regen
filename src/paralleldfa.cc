#ifdef __ENABLE_PARALLEL__
#include "paralleldfa.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace regen {

ParallelDFA::ParallelDFA(Expr *expr_root, const std::vector<StateExpr*> &state_exprs, std::size_t thread_num):
    nfa_size_(state_exprs.size()),
    dfa_size_(0),
    thread_num_(thread_num)
{
  typedef std::set<StateExpr*> NFA;
  fa_accepts_.resize(nfa_size_);
  for (NFA::iterator i = expr_root->transition().first.begin(); i != expr_root->transition().first.end(); ++i) {
    start_states_.insert((*i)->state_id());
  }

  ParallelTransition parallel_transition;
  std::map<ParallelTransition, int> pdfa_map;
  std::queue<ParallelTransition> queue;
  ParallelTransition::iterator iter;
  std::size_t pdfa_id = 0;

  pdfa_map[parallel_transition] = DFA::REJECT;

  for (std::size_t i = 0; i < nfa_size_; i++) {
    parallel_transition[i].insert(i);
  }

  pdfa_map[parallel_transition] = pdfa_id++;
  queue.push(parallel_transition);

  while (!queue.empty()) {
    parallel_transition = queue.front();
    parallel_transitions_.push_back(parallel_transition);
    queue.pop();
    std::vector<ParallelTransition> transition(256);

    iter = parallel_transition.begin();
    while (iter != parallel_transition.end()) {
      int start = (*iter).first;
      std::set<std::size_t> &currents = (*iter).second;
      for (std::set<std::size_t>::iterator i = currents.begin(); i != currents.end(); ++i) {
        StateExpr *s = state_exprs[*i];
        NFA &next = s->transition().follow;
        switch (s->type()) {
          case Expr::kLiteral: {
            Literal* literal = static_cast<Literal*>(s);
            unsigned char index = (unsigned char)literal->literal();
            for (NFA::iterator ni = next.begin(); ni != next.end(); ++ni) {
              transition[index][start].insert((*ni)->state_id());
            }
            break;
          }
          case Expr::kCharClass: {
            CharClass* charclass = static_cast<CharClass*>(s);
            for (int c = 0; c < 256; c++) {
              if (charclass->Involve(static_cast<unsigned char>(c))) {
                for (NFA::iterator ni = next.begin(); ni != next.end(); ++ni) {
                  transition[c][start].insert((*ni)->state_id());
                }
              }
            }
            break;
          }
          case Expr::kDot: {
            for (int c = 0; c < 256; c++) {
              for (NFA::iterator ni = next.begin(); ni != next.end(); ++ni) {
                transition[c][start].insert((*ni)->state_id());
              }
            }
            break;
          }
          case Expr::kBegLine: {
            for (NFA::iterator ni = next.begin(); ni != next.end(); ++ni) {
              transition['\n'][start].insert((*ni)->state_id());
            }
            break;
          }
          case Expr::kEndLine: {
            for (NFA::iterator ni = next.begin(); ni != next.end(); ++ni) {
              transition['\n'][start].insert((*ni)->state_id());
            }
          }
          case Expr::kEOP: {
            fa_accepts_[(s)->state_id()] = true;
            break;
          }
          case Expr::kNone: break;
          default: exitmsg("notype");
        }
      }
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
    set_state_info(false ,DFA::REJECT, dst_state);
  }
}

ParallelDFA::ParallelDFA(const DFA &dfa, std::size_t thread_num):
    nfa_size_(0),
    dfa_size_(dfa.size()),
    thread_num_(thread_num)
{
  fa_accepts_ = dfa.accepts();

  start_states_.insert(0);
  
  ParallelDFATransition parallel_transition;
  std::map<ParallelDFATransition, int> pdfa_map;
  std::queue<ParallelDFATransition> queue;
  ParallelDFATransition::iterator iter;
  std::size_t pdfa_id = 0;

  pdfa_map[parallel_transition] = DFA::REJECT;

  for (std::size_t i = 0; i < dfa_size_; i++) {
    parallel_transition[i] = i;
  }

  pdfa_map[parallel_transition] = pdfa_id++;
  queue.push(parallel_transition);

  while (!queue.empty()) {
    parallel_transition = queue.front();
    ParallelTransition parallel_transition_;
    for (iter = parallel_transition.begin(); iter != parallel_transition.end(); ++iter) {
      parallel_transition_[(*iter).first].insert((*iter).second);
    }
    parallel_transitions_.push_back(parallel_transition_);
    queue.pop();
    std::vector<ParallelDFATransition> transition(256);
    
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
      ++iter;
    }

    DFA::Transition &dfa_transition = get_new_transition();
    std::set<int> dst_state;
    bool has_reject = false;
    
    for (std::size_t c = 0; c < 256; c++) {
      ParallelDFATransition &next = transition[c];
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
    set_state_info(false ,DFA::REJECT, dst_state);
  }
}

void
ParallelDFA::FullMatchTask(TaskArg targ) const
{
  const unsigned char *str = targ.str;
  const unsigned char *end = targ.end;

  if (olevel_ >= O1) {
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
  std::size_t thread_num = thread_num_;
  if (end-str <= 2)  {
    thread_num = 1;
  } else if ((std::size_t)(end-str) < thread_num) {
    thread_num = end-str;
  }
  parallel_states_.resize(thread_num);
  boost::thread *threads[thread_num];
  std::size_t task_string_length = (end-str) / thread_num;
  std::size_t remainder_length = (end-str) % thread_num;
  const unsigned char *str_ = str, *end_;
  TaskArg targ;

  for (std::size_t i = 0; i < thread_num; i++) {
    end_ = str_ + task_string_length;
    if (i == thread_num - 1) end_ += remainder_length;
    targ.str = str_;
    targ.end = end_;
    targ.task_id = i;
    threads[i] = new boost::thread(
        boost::bind(
            boost::bind(&regen::ParallelDFA::FullMatchTask, this, _1),
            targ));
    str_ = end_;
  }

  std::set<std::size_t> states, next_states;
  states = start_states_;
  int pstate;

  for (std::size_t i = 0; i < thread_num; i++) {
    threads[i]->join();
    if ((pstate = parallel_states_[i]) == DFA::REJECT) {
      states.clear();
      break;
    }
    for (std::set<std::size_t>::iterator i = states.begin(); i != states.end(); ++i) {
      ParallelTransition::const_iterator iter = parallel_transitions_[pstate].find(*i);
      if (iter == parallel_transitions_[pstate].end()) continue;
      next_states.insert((*iter).second.begin(), (*iter).second.end());
    }
    states.swap(next_states);
    if (states.empty()) break;
    next_states.clear();
  }

  bool match = false;
  for (std::set<std::size_t>::iterator i = states.begin(); i != states.end(); ++i) {    if (fa_accepts_[*i]) {
      match = true;
      break;
    }
  }

  for (std::size_t i = 0; i < thread_num; i++) {
    threads[i]->join();
    delete threads[i];
  }
  
  return match;
}

} // namespace regen

#endif //__ENABLE_PARALLEL__
