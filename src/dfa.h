#ifndef REGEN_DFA_H_
#define  REGEN_DFA_H_
#include "regen.h"
#include "util.h"
#include "nfa.h"
#include "expr.h"
#if REGEN_ENABLE_JIT
#include "jitter.h"
#include "ext/xbyak/xbyak.h"
#include "ext/str_util.hpp"
#endif

namespace regen {
  
#if REGEN_ENABLE_JIT
class DFA;
class JITCompiler: public Xbyak::CodeGenerator {
 public:
  JITCompiler(const DFA &dfa, std::size_t state_code_size);
  std::size_t CodeSize() { return total_segment_size_; };
 private:
  std::size_t code_segment_size_;
  std::size_t data_segment_size_;
  std::size_t total_segment_size_;
  std::vector<const uint8_t *> filter_table_;
  std::vector<const uint8_t*> states_addr_;
  const uint8_t *filter_entry_;
  uint32_t reset_state_;
  static std::size_t code_segment_size(std::size_t state_num) {
    const std::size_t setup_code_size_ = 16;
    const std::size_t state_code_size_ = 64;
    const std::size_t segment_align = 4096;    
    return (state_num*state_code_size_ + setup_code_size_)
        +  ((state_num*state_code_size_ + setup_code_size_) % segment_align);
  }
  static std::size_t data_segment_size(std::size_t state_num) {
    return state_num * 256 * sizeof(void *);
  }
};
#endif

class Jitter;
class DFA {
public:
  typedef uint32_t state_t;
  typedef std::set<StateExpr*> Subset;
  enum StateType {
    REJECT = (state_t)-1,
    UNDEF  = (state_t)-2
  };
  struct Transition {
    state_t t[256];
    Transition(state_t fill = UNDEF) { std::fill(t, t+256, fill); }
    void fill(state_t fill) { std::fill(t, t+256, fill); }
    state_t &operator[](std::size_t index) { return t[index]; }
    const state_t &operator[](std::size_t index) const { return t[index]; }
  };
  struct AlterTrans {
    std::pair<unsigned char, unsigned char> key;
    state_t next1;
    state_t next2;
  };
  struct State {
    State(): transitions(NULL), accept(false), endline(false), id(UNDEF), inline_level(0) {}
    std::vector<Transition> *transitions;
    bool accept;
    bool endline;
    state_t id;
    std::set<state_t> dst_states;
    std::set<state_t> src_states;
    AlterTrans alter_transition;
    std::size_t inline_level;
    state_t &operator[](std::size_t index) { return (*transitions)[id][index]; }
    const state_t &operator[](std::size_t index) const { return (*transitions)[id][index]; }
  };
  typedef std::deque<State>::iterator iterator;
  typedef std::deque<State>::const_iterator const_iterator;

  DFA(const Regen::Options flag = Regen::Options::NoParseFlags): complete_(false), minimum_(false), flag_(flag), olevel_(Regen::Options::O0)
#ifdef REGEN_ENABLE_JIT
  , xgen_(NULL)
#endif
  {}
  DFA(const ExprInfo &expr_info, std::size_t limit = std::numeric_limits<size_t>::max());
  DFA(const NFA &nfa, std::size_t limit = std::numeric_limits<size_t>::max());
  #if REGEN_ENABLE_JIT
  virtual ~DFA() { delete xgen_; }
  #else
  virtual ~DFA() { }
  #endif
  
  bool empty() const { return transition_.empty(); }
  std::size_t size() const { return transition_.size(); }
  state_t start_state() const { return 0; }
  Regen::Options::CompileFlag olevel() const { return olevel_; };
  bool Complete() const { return complete_; }

  State& get_new_state() const;
  const ExprInfo &expr_info() const { return expr_info_; }
  void set_expr_info(const ExprInfo &expr_info) { expr_info_ = expr_info; }
  const Regen::Options &flag() const { return flag_; }
  std::size_t inline_level(std::size_t i) const { return states_[i].inline_level; }
  const std::set<state_t> &src_states(std::size_t i) const { return states_[i].src_states; }
  const std::set<state_t> &dst_states(std::size_t i) const { return states_[i].dst_states; }
  const AlterTrans &GetAlterTrans(std::size_t state) const { return states_[state].alter_transition; }
  const Transition &GetTransition(std::size_t state) const { return transition_[state]; }
  bool IsAcceptState(std::size_t state) const { return state == REJECT ? false : states_[state].accept; }
  bool IsEndlineState(std::size_t state) const { return state == REJECT ? false : states_[state].endline; }
  bool IsAcceptOrEndlineState(std::size_t state)  const { return IsAcceptState(state) | IsEndlineState(state); }

  bool ContainAcceptState(const Subset&) const;
  void ExpandStates(Subset*, bool begline = false, bool endline = false) const;
  void FillTransition(StateExpr*, std::vector<Subset>*) const;
  void MakeNonGreedy(StateExpr*) const;
  void TrimNonGreedy(Subset*) const;

  void Complementify();
  virtual bool Minimize();
  bool Compile(Regen::Options::CompileFlag olevel = Regen::Options::O2);
  virtual bool OnTheFlyMatch(const Regen::StringPiece& string, Regen::StringPiece* result = NULL) const;
  virtual bool Match(const Regen::StringPiece& string, Regen::StringPiece* result = NULL) const;
  void state2label(state_t state, char* labelbuf) const;

  bool Construct(std::size_t limit = std::numeric_limits<size_t>::max());
  bool Construct(const NFA &nfa, std::size_t limit = std::numeric_limits<size_t>::max());
  iterator begin() { return states_.begin(); }
  iterator end() { return states_.end(); }
  const_iterator begin() const { return states_.begin(); }
  const_iterator end() const { return states_.end(); }
  
  State &operator[](std::size_t index) { return states_[index]; }
  const State &operator[](std::size_t index) const { return states_[index]; }
  
protected:
  mutable std::vector<Transition> transition_;
  mutable std::deque<State> states_;
  mutable std::map<Subset, state_t> dfa_map_;
  mutable std::map<state_t, Subset> nfa_map_;
  ExprInfo expr_info_;
  mutable ExprPool pool_;
  mutable bool complete_;
  bool minimum_;
  Regen::Options flag_;
  void Finalize();
  state_t (*CompiledMatch)(const unsigned char**, const unsigned char**, state_t);
  bool EliminateBranch();
  bool Reduce();
  Regen::Options::CompileFlag olevel_;
#if REGEN_ENABLE_JIT
  JITCompiler *xgen_;
  mutable Jitter *jitter_;
#endif
  std::vector<AlterTrans> alter_trans_;
  std::vector<std::size_t> inline_level_;
private:
};

} // namespace regen
#endif // REGEN_DFA_H_
