#include "dfa.h"

namespace regen{

DFA::Transition& DFA::get_new_transition() {
  transition_.resize(transition_.size()+1);
  return transition_.back();
}

void DFA::set_state_info(bool accept, int default_next, std::set<int> &dst_state)
{
  accepts_.push_back(accept);
  defaults_.push_back(default_next);
  dst_states_.push_back(dst_state);
  std::set<int>::iterator iter = dst_state.begin();
  while (iter != dst_state.end()) {
    if (*iter == DFA::REJECT) {
      ++iter;
      continue;
    }
    if (src_states_.size() <= static_cast<std::size_t>(*iter)) {
      src_states_.resize(*iter+1);
    }
    src_states_[*iter].insert(dst_states_.size()-1);
    ++iter;
  }
}

void
DFA::Minimize()
{
  // TODO
  return;
}

void
DFA::Complement()
{
  int reject = DFA::REJECT;
  std::size_t final = size();
  for  (std::size_t state = 0; state < final; state++) {
    accepts_[state] = !accepts_[state];
    bool to_reject = false;
    for (std::size_t i = 0; i < 256; i++) {
      if (transition_[state][i] == DFA::REJECT) {
        if (reject == DFA::REJECT) {
          reject = transition_.size();
          transition_.push_back(Transition(reject));
          std::set<int> dst_states;
          dst_states.insert(reject);
          set_state_info(true, reject, dst_states);
        }
        to_reject = true;
        transition_[state][i] = reject;
      }
    }
    if (defaults_[state] == DFA::REJECT) {
      defaults_[state] = reject;
    }
    if (to_reject) {
      dst_states_[state].insert(reject);
      src_states_[reject].insert(state);
    }
  }
}

#if 1
class XbyakCompiler: public Xbyak::CodeGenerator {
 public:
  XbyakCompiler(const DFA &dfa, std::size_t state_code_size);
};

XbyakCompiler::XbyakCompiler(const DFA &dfa, std::size_t state_code_size = 32):
    /* dfa.size()*32 for code. <- code segment
     * padding for 4kb allign 
     * dfa.size()*256*sizeof(void *) for transition table. <- data segment */
    CodeGenerator(  ((dfa.size()*2)+1)*state_code_size
                  + (((dfa.size()*2)+1)*state_code_size % 4096)
                  + dfa.size()*256*sizeof(void *))
{
  std::vector<const uint8_t*> states_addr(dfa.size());

  const uint8_t* code_addr_top = getCurr();
  const uint8_t** transition_table_ptr = (const uint8_t **)(code_addr_top + ((dfa.size()*2)+1)*state_code_size + (((dfa.size()+3)*state_code_size) % 4096));

#ifdef XBYAK32
#error "64 only"
#elif defined(XBYAK64_WIN)
  const Xbyak::Reg64 arg1(rcx);
  const Xbyak::Reg64 arg2(rdx);
  const Xbyak::Reg64 tbl (r8);
  const Xbyak::Reg64 tmp1(r10);
  const Xbyak::Reg64 tmp2(r11);  
#else
  const Xbyak::Reg64 arg1(rdi);
  const Xbyak::Reg64 arg2(rsi);
  const Xbyak::Reg64 tbl (rdx);
  const Xbyak::Reg64 tmp1(r10);
  const Xbyak::Reg64 tmp2(r11);
#endif

  // setup enviroment on register
  mov(tbl,  (uint64_t)transition_table_ptr);
  jmp("s0");

  L("reject");
  const uint8_t *reject_state_addr = getCurr();
  mov(rax, -1); // return false
  ret();

  align(32);

  // state code generation, and indexing every states address.
  char labelbuf[1024];
  for (std::size_t i = 0; i < dfa.size(); i++) {
    sprintf(labelbuf, "s%"PRIdS, i);
    L(labelbuf);
    states_addr[i] = getCurr();
    const DFA::AlterTrans &at = dfa.GetAlterTrans(i);
    cmp(arg1, arg2);
    je("@f");
    // can transition without table lookup ?
    if (dfa.precompiled() && at.next1 != DFA::None) {
      if (at.next1 == DFA::REJECT) {
        strcpy(labelbuf, "reject");
      } else {
        sprintf(labelbuf, "s%d", at.next1); 
      }
      if (at.next2 == DFA::None) {
        inc(arg1);
        jmp(labelbuf, T_NEAR);
      } else {
        movzx(tmp1, byte[arg1]);
        inc(arg1);
        if (at.key.first == at.key.second) {
          cmp(tmp1, at.key.first);
          je(labelbuf, T_NEAR);
        } else {
          sub(tmp1, at.key.first);
          cmp(tmp1, at.key.second-at.key.first+1);
          jc(labelbuf, T_NEAR);
        }
        if (at.next2 == DFA::REJECT) {
          strcpy(labelbuf, "reject");
        } else {
          sprintf(labelbuf, "s%d", at.next2);
        }
        jmp(labelbuf, T_NEAR);
      }
    } else {
      movzx(tmp1, byte[arg1]);
      inc(arg1);
      jmp(ptr[tbl+i*256*8+tmp1*8]);
    }
    L("@@");
    mov(rax, i);
    ret();
    align(32);
  }

  // backpatching (every states address)
  for (std::size_t i = 0; i < dfa.size(); i++) {
    const DFA::Transition &trans = dfa.GetTransition(i);
    for (int c = 0; c < 256; c++) {
      int next = trans[c];
      transition_table_ptr[i*256+c] =
          (next  == -1 ? reject_state_addr : states_addr[next]);
    }
  }
}

bool DFA::PreCompile()
{
  alter_trans_.resize(size());
  for (std::size_t state = 0; state < size(); state++) {
    const Transition &trans = transition_[state];
    int next1 = trans[0], next2 = DFA::None;
    unsigned int begin = 0, end = 256;
    int c;
    for (c = 1; next1 == trans[c] && c < 256; c++);
    if (c < 256) {
      next2 = next1;
      next1 = trans[c];
      begin = c;
      for (++c; next1 == trans[c] && c < 256; c++);
    }
    if (c < 256) {
      end = --c;
      for (++c; next2 == trans[c] && c < 256; c++);
    }
    if (c < 256) {
      next1 = next2 = DFA::None;
    }
    AlterTrans &at = alter_trans_[state];
    at.next1 = next1;
    at.next2 = next2;
    at.key.first = begin;
    at.key.second = end;
  }

  precompiled_ = true;
  return true;
}

bool DFA::Compile()
{
  PreCompile();
  xgen_ = new XbyakCompiler(*this);
  CompiledFullMatch = (int (*)(const unsigned char *, const unsigned char *))xgen_->getCode();
  compiled_ = true;
  return true;
}
#else
bool DFA::PreCompile()
{
  precompiled_ = false;
  return true;
}
bool DFA::Compile()
{
  compiled_ = false;
  return true;
}
#endif

bool DFA::FullMatch(const unsigned char *str, const unsigned char *end) const
{
  int state = 0;

  if (compiled_) {
    state = CompiledFullMatch(str, end);
    return state != DFA::REJECT ? accepts_[state] : false;
  }

  while (str != end && (state = transition_[state][*str++]) != DFA::REJECT);

  return accepts_[state];
}

} // namespace regen
