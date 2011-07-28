#ifndef REGEN_DFA_H_
#define  REGEN_DFA_H_
#include "util.h"
#if __ENABLE_XBYAK__
#include <xbyak/xbyak.h>
#endif

namespace regen {

class DFA {
public:
  
  enum State {
    REJECT = -1,
    ACCEPT = -2,
    None   = -3
  };  

  struct Transition {
    int t[256];
    Transition(int fill = REJECT) { std::fill(t, t+256, fill); }
    int &operator[](std::size_t index) { return t[index]; }
    const int &operator[](std::size_t index) const { return t[index]; }
  };

  struct AlterTrans {
    std::pair<unsigned char, unsigned char> key;
    int next1;
    int next2;
  };

  DFA(): olevel_(O0) {}
  #if __ENABLE_XBYAK__
  ~DFA() { if (olevel_ >= O1) delete xgen_; }
  #endif
  
  bool empty() const { return transition_.empty(); }
  std::size_t size() const { return transition_.size(); }
  std::size_t start_state() const { return 0; }
  const std::deque<bool>& accepts() const { return accepts_; }
  std::size_t inline_level(std::size_t i) const { return inline_level_[i]; }
  const std::set<int> &src_states(std::size_t i) const { return src_states_[i]; }
  const std::set<int> &dst_states(std::size_t i) const { return dst_states_[i]; }
  Optimize olevel() const { return olevel_; };

  Transition& get_new_transition();
  void set_state_info(bool accept, int default_state, std::set<int> &dst_state);
  const AlterTrans &GetAlterTrans(std::size_t state) const { return alter_trans_[state]; }
  const Transition &GetTransition(std::size_t state) const { return transition_[state]; }
  int GetDefaultNext(std::size_t state) const { return defaults_[state]; }
  bool IsAcceptState(std::size_t state) const { return accepts_[state]; }
  void Complement();
  void Minimize();
  bool Compile(Optimize olevel = O2);
  virtual bool FullMatch(const std::string &str) const { return FullMatch((unsigned char*)str.c_str(), (unsigned char *)str.c_str()+str.length()); }
  virtual bool FullMatch(const unsigned char *str, const unsigned char *end) const;
  void int2label(int state, char* labelbuf) const;

protected:
  std::vector<Transition> transition_;
  std::deque<int> defaults_;
  std::deque<bool> accepts_;
  std::deque<std::set<int> > src_states_;
  std::deque<std::set<int> > dst_states_;
  int (*CompiledFullMatch)(const unsigned char *, const unsigned char *);
  bool EliminateBranch();
  bool Reduce();
  Optimize olevel_;
  #if __ENABLE_XBYAK__
  Xbyak::CodeGenerator *xgen_;
  #endif
  std::vector<AlterTrans> alter_trans_;
  std::vector<std::size_t> inline_level_;
};

} // namespace regen
#endif // REGEN_DFA_H_
