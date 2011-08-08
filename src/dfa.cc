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

void DFA::int2label(int state, char* labelbuf) const
{
  if (state == DFA::REJECT) {
    strcpy(labelbuf, "reject");
  } else {
    assert(0 <= state && state <= (int)size());
    sprintf(labelbuf, "s%d", state);
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

#if REGEN_ENABLE_XBYAK
class XbyakCompiler: public Xbyak::CodeGenerator {
 public:
  XbyakCompiler(const DFA &dfa, std::size_t state_code_size);
};

XbyakCompiler::XbyakCompiler(const DFA &dfa, std::size_t state_code_size = 64):
    /* dfa.size()*state_code_size for code. <- code segment
     * each states code was 16byte alligned.
     *                      ~~
     * padding for 4kb allign between code and data
     *                      ~~
     * dfa.size()*256*sizeof(void *) for transition table. <- data segment */
    CodeGenerator(  (dfa.size()+1)*state_code_size
                  + (dfa.size()+1)*state_code_size % 4096
                  + dfa.size()*256*sizeof(void *))
{
  std::vector<const uint8_t*> states_addr(dfa.size());

  const uint8_t* code_addr_top = getCurr();
  const uint8_t** transition_table_ptr = (const uint8_t **)(code_addr_top + (dfa.size()+1)*state_code_size + ((dfa.size()+1)*state_code_size) % 4096);

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

  align(16);

  // state code generation, and indexing every states address.
  char labelbuf[100];
  for (std::size_t i = 0; i < dfa.size(); i++) {
    dfa.int2label(i, labelbuf);
    L(labelbuf);
    states_addr[i] = getCurr();
    // can transition without table lookup ?
    
    const DFA::AlterTrans &at = dfa.GetAlterTrans(i);
    if (dfa.olevel() >= O2 && at.next1 != DFA::None) {
      std::size_t state = i;
      std::size_t inline_level = dfa.olevel() == O3 ? dfa.inline_level(i) : 0;
      bool inlining = inline_level != 0;
      std::size_t transition_depth = -1;
      inLocalLabel();
      if (inlining) {
        lea(tmp1, ptr[arg1+inline_level]);
        cmp(tmp1, arg2);
        jge(".ret", T_NEAR);
      } else {
        cmp(arg1, arg2);
        je(".ret", T_NEAR);        
      }
   emit_transition:
      const DFA::AlterTrans &at = dfa.GetAlterTrans(state);
      bool jn_flag = false;
      transition_depth++;
      assert(at.next1 != DFA::None);
      dfa.int2label(at.next1, labelbuf);
      if (at.next1 != DFA::REJECT) {
        jn_flag = true;  
      }
      if (at.next2 == DFA::None) {
        if (!inlining) {
          inc(arg1);
          jmp(labelbuf, T_NEAR);
        } else if (transition_depth == inline_level) {
          add(arg1, transition_depth);
          jmp(labelbuf, T_NEAR);
        } else {
          state = at.next1;
          goto emit_transition;
        }
      } else {
        if (!inlining) {
          movzx(tmp1, byte[arg1]);
          inc(arg1);
        } else if (transition_depth == inline_level) {
          movzx(tmp1, byte[arg1+transition_depth]);
          add(arg1, transition_depth+1);
        } else {
          movzx(tmp1, byte[arg1+transition_depth]);
        }
        if (at.key.first == at.key.second) {
          cmp(tmp1, at.key.first);
          if (transition_depth == inline_level || !jn_flag) {
            je(labelbuf, T_NEAR);
          } else {
            jne("reject", T_NEAR);
          }
        } else {
          sub(tmp1, at.key.first);
          cmp(tmp1, at.key.second-at.key.first+1);
          if (transition_depth == inline_level || !jn_flag) {
            jc(labelbuf, T_NEAR);
          } else {
            jnc("reject", T_NEAR);
          }
        }
        dfa.int2label(at.next2, labelbuf);
        if (transition_depth == inline_level) {
          jmp(labelbuf, T_NEAR);
        } else {
          if (jn_flag) {
            state = at.next1;
            goto emit_transition;
          } else {
            state = at.next2;
            goto emit_transition;
          }
        }
      }
      if (inlining) {
        L(".ret");
        cmp(arg1, arg2);
        je("@f", T_NEAR);
        movzx(tmp1, byte[arg1]);
        inc(arg1);
        jmp(ptr[tbl+i*256*8+tmp1*8]);
        L("@@");
        mov(rax, i);
        ret();
      } else {
        L(".ret");
        mov(rax, i);
        ret();
      }
      outLocalLabel();
    } else {
      cmp(arg1, arg2);
      je("@f");
      movzx(tmp1, byte[arg1]);
      inc(arg1);
      jmp(ptr[tbl+i*256*8+tmp1*8]);
      L("@@");
      mov(rax, i);
      ret();
    }
    align(16);
  }

  // backpatching (each states address)
  for (std::size_t i = 0; i < dfa.size(); i++) {
    const DFA::Transition &trans = dfa.GetTransition(i);
    for (int c = 0; c < 256; c++) {
      int next = trans[c];
      transition_table_ptr[i*256+c] =
          (next  == -1 ? reject_state_addr : states_addr[next]);
    }
  }
}

bool DFA::EliminateBranch()
{
  alter_trans_.resize(size());
  for (std::size_t state = 0; state < size(); state++) {
    const Transition &trans = transition_[state];
    int next1 = trans[0], next2 = DFA::None;
    unsigned int begin = 0, end = 256;
    int c;
    for (c = 1; c < 256 && next1 == trans[c]; c++);
    if (c < 256) {
      next2 = next1;
      next1 = trans[c];
      begin = c;
      for (++c; c < 256 && next1 == trans[c]; c++);
    }
    if (c < 256) {
      end = --c;
      for (++c; c < 256 && next2 == trans[c]; c++);
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

  return true;
}

bool DFA::Reduce()
{
  inline_level_.resize(size());
  src_states_[0].insert(DFA::None);
  std::vector<bool> inlined(size());
  for (std::size_t state = 0; state < size(); state++) {
    // Pick inlining region (make degenerate graph).
    if (inlined[state]) continue;
    int current = state, next;
    for(;;) {
      if (dst_states_[current].size() > 2 ||
          dst_states_[current].size() == 0) break;
      if (dst_states_[current].size() == 2 &&
          dst_states_[current].count(DFA::REJECT) == 0) break;
      if (dst_states_[current].size() == 1 &&
          dst_states_[current].count(DFA::REJECT) == 1) break;
      next = *(dst_states_[current].lower_bound(0));
      if (alter_trans_[next].next1 == DFA::None) break;
      if (src_states_[next].size() != 1 ||
          accepts_[next]) break;
      if (inlined[next]) break;
      inlined[next] = true;
      current = next;

      inline_level_[state]++;
    }
  }
  src_states_[0].erase(DFA::None);

  return true;
}

bool DFA::Compile(Optimize olevel)
{
  if (olevel <= olevel_) return true;
  if (olevel >= O2) {
    if (EliminateBranch()) {
      olevel_ = O2;
    }
    if (olevel == O3 && Reduce()) {
      olevel_ = O3;
    }
  }
  xgen_ = new XbyakCompiler(*this);
  CompiledFullMatch = (int (*)(const unsigned char *, const unsigned char *))xgen_->getCode();
  if (olevel_ < O1) olevel_ = O1;
  return olevel == olevel_;
}
#else
bool DFA::EliminateBranch() { return false; }
bool DFA::Reduce() { return false; }
bool DFA::Compile(Optimize) { return false; }
#endif

bool DFA::FullMatch(const unsigned char *str, const unsigned char *end) const
{
  int state = 0;

  if (olevel_ >= O1) {
    state = CompiledFullMatch(str, end);
    return state != DFA::REJECT ? accepts_[state] : false;
  }

  while (str != end && (state = transition_[state][*str++]) != DFA::REJECT);
  return state != DFA::REJECT ? accepts_[state] : false;
}

} // namespace regen
