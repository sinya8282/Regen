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
    if (src_states_.size() <= static_cast<std::size_t>(*iter)) {
      src_states_.resize(*iter+1);
    }
    src_states_[*iter].insert(src_states_[*iter].begin(), dst_states_.size()-1);
    ++iter;
  }
}

void DFA::Negative()
{
  int reject = DFA::REJECT;
  std::size_t final = transition_.size();
  for  (std::size_t state = 0; state < final; state++) {
    accepts_[state] = !accepts_[state];
    bool to_reject = false;
    for (std::size_t i = 0; i < 256; i++) {
      if (transition_[state][i] == DFA::REJECT) {
        if (reject == DFA::REJECT) {
          reject = transition_.size();
          transition_.push_back(Transition(reject));
          std::set<int> dst_states;
          dst_states.insert(reject);
          set_state_info(true, reject, dst_states);
        }
        to_reject = true;
        transition_[state][i] = reject;
      }
    }
    if (defaults_[state] == DFA::REJECT) {
      defaults_[state] = reject;
    }
    if (to_reject) {
      dst_states_[state].insert(reject);
      src_states_[reject].insert(state);
    }
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
