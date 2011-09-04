#ifndef REGEN_NFA_H_
#define  REGEN_NFA_H_
#include "util.h"

namespace regen {

class NFA {
public:
  typedef uint32_t state_t;
  struct State {
    std::set<state_t> transition[256];
    std::size_t id;
    bool accept;
    std::set<state_t> &operator[](std::size_t index) { return transition[index]; }
    const std::set<state_t> &operator[](std::size_t index) const { return transition[index]; }
  };
  typedef std::deque<State>::iterator iterator;
  typedef std::deque<State>::const_iterator const_iterator;  

  bool empty() const { return states_.empty(); }
  std::size_t size() const { return states_.size(); }
  std::set<state_t>& start_states() { return start_states_; }
  const std::set<state_t>& start_states() const { return start_states_; }
  State& get_new_state();

  iterator begin() { return states_.begin(); }
  iterator end() { return states_.end(); }
  const_iterator begin() const { return states_.begin(); }
  const_iterator end() const { return states_.end(); }

  State &operator[](std::size_t index) { return states_[index]; }
  const State &operator[](std::size_t index) const { return states_[index]; }

protected:
  std::deque<State> states_;
  std::set<state_t> start_states_;
};

} // namespace regen
#endif // REGEN_NFA_H_
