#include "dfa.h"

namespace regen{

DFA::Transition& DFA::get_new_transition() {
  transition_.resize(transition_.size()+1);
  return transition_.back();
}

void DFA::set_state_info(bool accept, int default_next, std::set<int> &dst_state)
{
  accepts_.push_back(accept);
  defaults_.push_back(default_next);
  dst_states_.push_back(dst_state);
  std::set<int>::iterator iter = dst_state.begin();
  while (iter != dst_state.end()) {
    if (src_states_.size() <= *iter) {
      src_states_.resize(*iter+1);
    }
    src_states_[*iter].insert(src_states_[*iter].begin(), dst_states_.size()-1);
    ++iter;
  }
}

bool DFA::FullMatch(const unsigned char *str, const unsigned char *end) const
{
  int state = 0, next;
  while (str != end && (next = transition_[state][*str++]) != DFA::REJECT) {
    state = next;
  }

  return accepts_[state];
}

} // namespace regen
