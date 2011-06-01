#include "dfa.h"

namespace regen{

DFA::Transition& DFA::get_new_transition() {
  transition_.resize(transition_.size()+1);
  return transition_.back();
}

void DFA::set_state_info(bool accept, int default_next)
{
  accepts_.push_back(accept);
  defaults_.push_back(default_next);
}

bool DFA::IsMatched(const unsigned char *str, const unsigned char *end) const
{
  int state = 0, next;
  while(str != end && (next = transition_[state][*str++]) != DFA::REJECT) {
    state = next;
  }

  return accepts_[state];
}

} // namespace regen
