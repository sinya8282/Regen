#include "dfa.h"

namespace regen{

DFA::~DFA() {}

void DFA::set_transition(std::vector<int>*tbl, bool accept, int default_next)
{
  transition_.push_back(tbl);
  accepts_.push_back(accept);
  defaults_.push_back(default_next);
}

bool DFA::Matching(const unsigned char *str, const unsigned char *end)
{
  int state = 0, next;
  while(str != end && (next = (*transition_[state])[*str++]) != DFA::REJECT) {
    if (accepts_[next]) return true;
    state = next;
  }

  return accepts_[state];
}

} // namespace regen
