#include "nfa.h"

namespace regen{

NFA::State& NFA::get_new_state() {
  states_.resize(states_.size()+1);
  State &s = states_.back();
  s.id = states_.size()-1;
  s.accept = false;
  return s;
}

} // namespace regen
