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
  };

  bool empty() const { return states_.empty(); }
  std::size_t size() const { return states_.size(); }
  std::set<state_t> start_states() const { return start_states_; }
  State& get_new_state();

protected:
  std::vector<State> states_;
  std::set<state_t> start_states_;
};

} // namespace regen
#endif // REGEN_NFA_H_
