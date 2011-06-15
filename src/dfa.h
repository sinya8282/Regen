#ifndef REGEN_DFA_H_
#define  REGEN_DFA_H_
#include "util.h"

namespace regen {

class DFA {
public:
  
  enum State {
    REJECT = -1,
    ACCEPT = -2
  };  

  struct Transition {
    int t[256];
    Transition(int fill = REJECT) { std::fill(t, t+256, fill); }
    int &operator[](std::size_t index) { return t[index]; }
    const int &operator[](std::size_t index) const { return t[index]; }
  };
  
  bool empty() const { return transition_.empty(); }
  std::size_t size() const { return transition_.size(); }
  std::size_t start_state() const { return 0; }
  std::size_t max_length() const { return max_length_; }
  std::size_t min_length() const { return min_length_; }
  std::size_t must_max_length() const { return must_max_length_; }
  const std::string& must_max_word() const { return must_max_word_; }
  std::set<int> &src_state(std::size_t i) { return src_states_[i]; }
  std::set<int> &dst_state(std::size_t i) { return dst_states_[i]; }

  void set_max_length(std::size_t len) { max_length_ = len; }
  void set_min_length(std::size_t len) { min_length_ = len; }
  void set_must_max_length(std::size_t len) { must_max_length_ = len; }
  void set_must_max_word(const std::string& string) { must_max_word_ = string; }
  Transition& get_new_transition();
  void set_state_info(bool accept, int default_state, std::set<int> &dst_state);

  bool FullMatch(const std::string &str) const { return FullMatch((unsigned char*)str.c_str(), (unsigned char *)str.c_str()+str.length()); }
  bool FullMatch(const unsigned char *str, const unsigned char *end) const;
  const Transition &GetTransition(std::size_t state) const { return transition_[state]; }
  int GetDefaultNext(std::size_t state) const { return defaults_[state]; }
  bool IsAcceptState(std::size_t state) const { return accepts_[state]; }

private:
  std::vector<Transition> transition_;
  std::deque<int> defaults_;
  std::deque<bool> accepts_;
  std::deque<std::set<int> > src_states_;
  std::deque<std::set<int> > dst_states_;
  std::size_t max_length_;
  std::size_t min_length_;
  std::size_t must_max_length_;
  std::string must_max_word_;
};

} // namespace regen
#endif // REGEN_DFA_H_
