#ifndef REGEN_DFA_H_
#define  REGEN_DFA_H_
#include "regen.h"
#include "util.h"
#include "nfa.h"
#include "expr.h"
#if REGEN_ENABLE_XBYAK
#include <xbyak/xbyak.h>
#endif

namespace regen {
  
#if REGEN_ENABLE_XBYAK
class DFA;
class JITCompiler: public Xbyak::CodeGenerator {
 public:
  JITCompiler(const DFA &dfa, std::size_t state_code_size);
  std::size_t CodeSize() { return total_segment_size_; };
 private:
  std::size_t code_segment_size_;
  std::size_t data_segment_size_;
  std::size_t total_segment_size_;
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
class DFA {
#ifdef REGEN_ENABLE_XBYAK
  class Jitter: public Xbyak::CodeGenerator {
    friend class DFA;
    struct Transition {
      void* t[256];
      Transition(void* fill = NULL) { std::fill(t, t+256, fill); }
      void* &operator[](std::size_t index) { return t[index]; }
    };
 public:
    Jitter(std::size_t code_size = 4096): CodeGenerator(code_size), state_num_(0) {}
 private:
    std::deque<Transition> data_segment_;
    std::size_t state_num_;
};
#endif  
public:
  typedef uint32_t state_t;
  enum StateType {
    REJECT = (state_t)-1,
    UNDEF  = (state_t)-2
  };
  struct Transition {
    state_t t[256];
    Transition(state_t fill = UNDEF) { std::fill(t, t+256, fill); }
    state_t &operator[](std::size_t index) { return t[index]; }
    const state_t &operator[](std::size_t index) const { return t[index]; }
  };
  struct AlterTrans {
    std::pair<unsigned char, unsigned char> key;
    state_t next1;
    state_t next2;
  };
  struct State {
   State(): transitions(NULL), accept(false), id(UNDEF), inline_level(0) {}
    std::vector<Transition> *transitions;
    bool accept;
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
#ifdef REGEN_ENABLE_XBYAK
  , xgen_(NULL)
#endif
  {}
  DFA(Expr *expr_root, std::size_t limit = std::numeric_limits<size_t>::max());
  DFA(const NFA &nfa, std::size_t limit = std::numeric_limits<size_t>::max());
  #if REGEN_ENABLE_XBYAK
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
  const Expr* expr_root() const { return expr_root_; }
  void set_expr_root(Expr* expr_root) { expr_root_ = expr_root; }
  const Regen::Options &flag() const { return flag_; }
  std::size_t inline_level(std::size_t i) const { return states_[i].inline_level; }
  const std::set<state_t> &src_states(std::size_t i) const { return states_[i].src_states; }
  const std::set<state_t> &dst_states(std::size_t i) const { return states_[i].dst_states; }
  const AlterTrans &GetAlterTrans(std::size_t state) const { return states_[state].alter_transition; }
  const Transition &GetTransition(std::size_t state) const { return transition_[state]; }
  bool IsAcceptState(std::size_t state) const { return states_[state].accept; }

  bool Construct(Expr *expr_root, std::size_t limit = std::numeric_limits<size_t>::max());
  bool Construct(const NFA &nfa, std::size_t limit = std::numeric_limits<size_t>::max());
  state_t OnTheFlyConstructWithChar(state_t state, unsigned char input, Regen::Context *context) const;
  std::pair<state_t, const unsigned char *> OnTheFlyConstructWithString(state_t state, const unsigned char *begin, const unsigned char *end, Regen::Context *context) const;
  void Complement();
  virtual bool Minimize();
  bool Compile(Regen::Options::CompileFlag olevel = Regen::Options::O2);
  virtual bool OnTheFlyMatch(const std::string &str, Regen::Context *context) const { return OnTheFlyMatch((unsigned char*)str.c_str(), (unsigned char *)str.c_str()+str.length(), context); }
  virtual bool OnTheFlyMatch(const unsigned char *, const unsigned char*, Regen::Context *context) const;
  virtual bool Match(const std::string &str, Regen::Context *context = NULL) const { return Match((unsigned char*)str.c_str(), (unsigned char *)str.c_str()+str.length(), context); }
  virtual bool Match(const unsigned char *str, const unsigned char *end, Regen::Context *context = NULL) const;
  void state2label(state_t state, char* labelbuf) const;

  iterator begin() { return states_.begin(); }
  iterator end() { return states_.end(); }
  const_iterator begin() const { return states_.begin(); }
  const_iterator end() const { return states_.end(); }
  
  State &operator[](std::size_t index) { return states_[index]; }
  const State &operator[](std::size_t index) const { return states_[index]; }
  
protected:
  mutable std::vector<Transition> transition_;
  mutable std::deque<State> states_;
  mutable std::map<std::set<StateExpr*>, state_t> dfa_map_;
  mutable std::map<state_t, std::set<StateExpr*> > nfa_map_;
  Expr* expr_root_;
  bool complete_;
  bool minimum_;
  Regen::Options flag_;
  void Finalize();
  state_t (*CompiledMatch)(const unsigned char *, const unsigned char *, Regen::Context*);
  bool EliminateBranch();
  bool Reduce();
  Regen::Options::CompileFlag olevel_;
#if REGEN_ENABLE_XBYAK
  JITCompiler *xgen_;
#endif
  std::vector<AlterTrans> alter_trans_;
  std::vector<std::size_t> inline_level_;
private:
};

} // namespace regen
#endif // REGEN_DFA_H_
