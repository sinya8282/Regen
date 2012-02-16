#ifndef REGEN_JITTER_H_
#define  REGEN_JITTER_H_

#include <xbyak/xbyak.h>
#include <vector>
#include <list>

namespace regen {

class Jitter;
class DFA;
  
class CodeSegment: public Xbyak::CodeGenerator {
public:
  CodeSegment(const DFA &dfa, Jitter &jitter, std::size_t code_segment_size);
  const void* EmitFunc();
  const void* EmitState(std::size_t state);
  const DFA &dfa_;
  Jitter &jitter_;
  std::size_t CodeSegmentSize() { return code_segment_size_; };
  std::size_t code_segment_size_;
  const void *reject_addr_;
  const void *return_addr_;
#ifdef XBYAK32
  const Xbyak::Reg32& arg1;
  const Xbyak::Reg32& arg2;
  const Xbyak::Reg32& arg3;
  const Xbyak::Reg32& tbl;
  const Xbyak::Reg32& tmp1;
  const Xbyak::Reg32& tmp2;
  const Xbyak::Reg32& reg_a;
#else
  const Xbyak::Reg64& arg1;
  const Xbyak::Reg64& arg2;
  const Xbyak::Reg64& arg3;
  const Xbyak::Reg64& tbl;
  const Xbyak::Reg64& tmp1;
  const Xbyak::Reg64& tmp2;
  const Xbyak::Reg64& reg_a;
#endif
};

class Jitter {
public:
  struct Transition {
    void* t[256];
    Transition(void* fill = NULL) { std::fill(t, t+256, fill); }
    void* &operator[](std::size_t index) { return t[index]; }
  };
  struct StateInfo {
    StateInfo(Transition &t): addr(NULL), transition(t) {}
    const void *addr;
    Transition &transition;
  };
  Jitter(const DFA &dfa, std::size_t code_segment_size = 4096): dfa_(dfa), code_segment_size_(code_segment_size), func_ptr_(NULL)  {}
  ~Jitter() { for (std::vector<CodeSegment *>::iterator i = code_segments_.begin(); i != code_segments_.end(); ++i) delete *i; }

  CodeSegment * CS() { return code_segments_.back(); }
  CodeSegment * NewCS() { code_segments_.push_back(0); code_segments_.back() = new CodeSegment(dfa_, *this, code_segment_size_); return code_segments_.back(); }
  StateInfo &state_info(std::size_t state) { return state_info_[state]; }
  void Init();
private:
  const DFA &dfa_;
  std::size_t code_segment_size_;
  std::vector<CodeSegment *> code_segments_;
  std::list<Transition> data_segment_;
  std::vector<StateInfo> state_info_;
  std::size_t state_num_;
  const void *func_ptr_;
};

} // namespace regen
#endif // REGEN_JITTER_H_
