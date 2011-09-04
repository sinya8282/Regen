#ifndef REGEN_DFA_H_
#define  REGEN_DFA_H_
#include "util.h"
#include "nfa.h"
#include "expr.h"
#if REGEN_ENABLE_XBYAK
#include <xbyak/xbyak.h>
#endif

namespace regen {

class DFA {
public:
  typedef uint32_t state_t;
  enum State {
    REJECT = (state_t)-1,
    ACCEPT = (state_t)-2,
    NONE   = (state_t)-3
  };
  struct Transition {
    state_t t[256];
    Transition(state_t fill = REJECT) { std::fill(t, t+256, fill); }
    state_t &operator[](std::size_t index) { return t[index]; }
    state_t operator[](std::size_t index) const { return t[index]; }
  };
  struct AlterTrans {
    std::pair<unsigned char, unsigned char> key;
    state_t next1;
    state_t next2;
  };

  DFA(): complete_(false), minimum_(false), olevel_(O0), xgen_(NULL) {}
  DFA(Expr *expr_root, std::size_t limit = std::numeric_limits<size_t>::max(), std::size_t neop = 1);
  DFA(const NFA &nfa, std::size_t limit = std::numeric_limits<size_t>::max());
  #if REGEN_ENABLE_XBYAK
  ~DFA() { if (xgen_ != NULL) delete xgen_; }
  #endif
  
  bool empty() const { return transition_.empty(); }
  std::size_t size() const { return transition_.size(); }
  state_t start_state() const { return 0; }
  const std::vector<bool>& accepts() const { return accepts_; }
  std::size_t inline_level(std::size_t i) const { return inline_level_[i]; }
  const std::set<state_t> &src_states(std::size_t i) const { return src_states_[i]; }
  const std::set<state_t> &dst_states(std::size_t i) const { return dst_states_[i]; }
  Optimize olevel() const { return olevel_; };
  bool Complete() const { return complete_; }

  Transition& get_new_transition();
  void set_state_info(bool accept, state_t default_state, std::set<state_t> &dst_state);
  const AlterTrans &GetAlterTrans(std::size_t state) const { return alter_trans_[state]; }
  const Transition &GetTransition(std::size_t state) const { return transition_[state]; }
  state_t GetDefaultNext(std::size_t state) const { return defaults_[state]; }
  bool IsAcceptState(std::size_t state) const { return accepts_[state]; }
  bool Construct(Expr *expr_root, std::size_t limit = std::numeric_limits<size_t>::max(), std::size_t neop = 1);
  bool Construct(const NFA &nfa, std::size_t limit = std::numeric_limits<size_t>::max());
  void Complement();
  virtual void Minimize();
  bool Compile(Optimize olevel = O2);
  virtual bool FullMatch(const std::string &str) const { return FullMatch((unsigned char*)str.c_str(), (unsigned char *)str.c_str()+str.length()); }
  virtual bool FullMatch(const unsigned char *str, const unsigned char *end) const;
  void state2label(state_t state, char* labelbuf) const;

protected:
  std::vector<Transition> transition_;
  std::vector<state_t> defaults_;
  std::vector<bool> accepts_;
  std::vector<std::set<state_t> > src_states_;
  std::vector<std::set<state_t> > dst_states_;
  bool complete_;
  bool minimum_;
  state_t (*CompiledFullMatch)(const unsigned char *, const unsigned char *);
  bool EliminateBranch();
  bool Reduce();
  Optimize olevel_;
  #if REGEN_ENABLE_XBYAK
  Xbyak::CodeGenerator *xgen_;
  #endif
  std::vector<AlterTrans> alter_trans_;
  std::vector<std::size_t> inline_level_;
};

} // namespace regen
#endif // REGEN_DFA_H_
