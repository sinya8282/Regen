#include "dfa.h"

namespace regen{

DFA::DFA(Expr *expr_root, std::size_t limit, std::size_t neop):
    complete_(false), minimum_(false), olevel_(O0), xgen_(NULL)
{
  complete_ = Construct(expr_root, limit, neop);
}

DFA::DFA(const NFA &nfa, std::size_t limit):
    complete_(false), minimum_(false), olevel_(O0), xgen_(NULL)
{
  complete_ = Construct(nfa, limit);
}

bool DFA::Construct(Expr *expr_root, std::size_t limit, std::size_t neop)
{
  state_t dfa_id = 0;

  typedef std::set<StateExpr*> NFA;
  
  std::map<NFA, int> dfa_map;
  std::queue<NFA> queue;
  NFA default_next;
  NFA first_states = expr_root->transition().first;

  dfa_map[default_next] = REJECT;
  dfa_map[first_states] = dfa_id++;
  queue.push(first_states);

  while (!queue.empty()) {
    NFA nfa_states = queue.front();
    queue.pop();
    std::vector<NFA> transition(256);
    NFA::iterator iter = nfa_states.begin();
    bool is_accept = false;
    default_next.clear();
    std::set<EOP*> eops;

    while (iter != nfa_states.end()) {
      NFA &next = (*iter)->transition().follow;
      switch ((*iter)->type()) {
        case Expr::kLiteral: {
          Literal* literal = static_cast<Literal*>(*iter);
          unsigned char index = (unsigned char)literal->literal();
          transition[index].insert(next.begin(), next.end());
          break;
        }
        case Expr::kCharClass: {
          CharClass* charclass = static_cast<CharClass*>(*iter);
          for (int i = 0; i < 256; i++) {
            if (charclass->Involve(static_cast<unsigned char>(i))) {
              transition[i].insert(next.begin(), next.end());
            }
          }
          break;
        }
        case Expr::kDot: {
          for (int i = 0; i < 256; i++) {
            transition[i].insert(next.begin(), next.end());
          }
          default_next.insert(next.begin(), next.end());
          break;
        }
        case Expr::kBegLine: {
          transition['\n'].insert(next.begin(), next.end());
          break;
        }
        case Expr::kEndLine: {
          transition['\n'].insert(next.begin(), next.end());
          break;
        }
        case Expr::kEOP: {
          eops.insert((EOP*)(*iter));
          if (eops.size() == neop) {
            is_accept = true;
          }
          break;
        }
        case Expr::kNone: break;
        default: exitmsg("notype");
      }
      ++iter;
    }

    State &state = get_new_state();
    Transition &trans = transition_[state.id];
    bool has_reject = false;
    // only Leftmost-Shortest matching
    //if (is_accept) goto settransition;
    
    for (state_t i = 0; i < 256; i++) {
      NFA &next = transition[i];
      if (next.empty()) {
        has_reject = true;
        continue;
      }

      if (dfa_map.find(next) == dfa_map.end()) {
        dfa_map[next] = dfa_id++;
        if (dfa_id > (state_t)limit) {
          return false;
        }
        queue.push(next);
      }
      trans[i] = dfa_map[next];
      state.dst_states.insert(dfa_map[next]);
    }
    //settransition:
    if (has_reject) state.dst_states.insert(REJECT);
    state.accept = is_accept;
    state.default_next = dfa_map[default_next];
  }

  Finalize();
  
  complete_ = true;
  return true;
}

bool DFA::Construct(const NFA &nfa, size_t limit)
{
  state_t dfa_id = 0;

  typedef std::set<NFA::state_t> NFA_;

  std::map<std::set<NFA::state_t>, state_t> dfa_map;
  std::queue<NFA_> queue;
  const NFA_ &start_states = nfa.start_states();
  
  dfa_map[start_states] = dfa_id++;
  queue.push(start_states);

  while (!queue.empty()) {
    NFA_ nfa_states = queue.front();
    queue.pop();
    std::vector<NFA_> transition(256);
    bool is_accept = false;

    for (NFA_::iterator iter = nfa_states.begin(); iter != nfa_states.end(); ++iter) {
      const NFA::State &state = nfa[*iter];
      for (std::size_t i = 0; i < 256; i++) {
        transition[i].insert(state.transition[i].begin(), state.transition[i].end());
      }
      is_accept |= state.accept;
    }

    State &state = get_new_state();
    Transition &trans = transition_[state.id];
    bool has_reject = false;
    // only Leftmost-Shortest matching
    //if (is_accept) goto settransition;
    
    for (state_t i = 0; i < 256; i++) {
      NFA_ &next = transition[i];
      if (next.empty()) {
        has_reject = true;
        continue;
      }

      if (dfa_map.find(next) == dfa_map.end()) {
        dfa_map[next] = dfa_id++;
        queue.push(next);
      }
      trans[i] = dfa_map[next];
      state.dst_states.insert(dfa_map[next]);
    }
    //settransition:
    if (has_reject) state.dst_states.insert(REJECT);
    state.accept = is_accept;
  }

  Finalize();

  complete_ = true;
  return true;
}

void DFA::Finalize()
{
  for (iterator state_iter = begin(); state_iter != end(); ++state_iter) {
    for (std::set<state_t>::iterator iter = state_iter->dst_states.begin();
         iter != state_iter->dst_states.end(); ++iter) {
      if (*iter != REJECT) {
        states_[*iter].src_states.insert(state_iter->id);
      }
    }
  }
}

DFA::State& DFA::get_new_state() {
  transition_.resize(states_.size()+1);
  states_.resize(states_.size()+1);
  State &new_state = states_.back();
  new_state.transitions = &transition_;
  new_state.id = states_.size()-1;
  new_state.alter_transition.next1 = NONE;
  return new_state;
}

void DFA::state2label(state_t state, char* labelbuf) const
{
  if (state == REJECT) {
    strcpy(labelbuf, "reject");
  } else {
    assert(0 <= state && state <= (state_t)size());
    sprintf(labelbuf, "s%d", state);
  }
}

void
DFA::Minimize()
{
  if (minimum_) return;
  
  std::vector<std::vector<bool> > distinction_table;
  distinction_table.resize(size()-1);
  for (state_t i = 0; i < size()-1; i++) {
    distinction_table[i].resize(size()-i-1);
    for (state_t j = i+1; j < size(); j++) {
      distinction_table[i][size()-j-1] = states_[i].accept != states_[j].accept;
    }
  }

  bool distinction_flag = true;
  while (distinction_flag) {
    distinction_flag = false;
    for (state_t i = 0; i < size()-1; i++) {
      for (state_t j = i+1; j < size(); j++) {
        if (!distinction_table[i][size()-j-1]) {
          for (std::size_t input = 0; input < 256; input++) {
            state_t n1, n2;
            n1 = transition_[i][input];
            n2 = transition_[j][input];
            if (n1 != n2) {
              if (n1 > n2) std::swap(n1, n2);
              if ((n1 == REJECT || n2 == REJECT) ||
                  distinction_table[n1][size()-n2-1]) {
                distinction_flag = true;
                distinction_table[i][size()-j-1] = true;
                break;
              }
            }
          }
        }
      }
    }
  }
  
  std::map<state_t, state_t> swap_map;
  for (state_t i = 0; i < size()-1; i++) {
    for (state_t j = i+1; j < size(); j++) {
      if (swap_map.find(j) == swap_map.end()) {
        if (!distinction_table[i][size()-j-1]) {
          swap_map[j] = i;
        }
      }
    }
  }

  if (swap_map.empty()) {
    minimum_ = true;
    return;
  }

  size_t minimum_size = size() - swap_map.size();
  std::vector<state_t> replace_map(size());
  for (state_t s = 0, d = 0; s < size(); s++) {
    if (swap_map.find(s) == swap_map.end()) {
      replace_map[s] = d++;
      if (s != replace_map[s]) {
        transition_[replace_map[s]] = transition_[s];
        states_[replace_map[s]] = states_[s];
        states_[s].id = s;
      }
    } else {
      replace_map[s] = replace_map[swap_map[s]];
    }
  }

  std::set<state_t> tmp_set;
  for (iterator state_iter = begin(); state_iter->id < minimum_size; ++state_iter) {
    State &state = *state_iter;
    for (std::size_t input = 0; input < 256; input++) {
      state_t n = state[input];
      if (n != REJECT) {
        state[input] = replace_map[n];
      }
    }
    if (state.default_next != REJECT) state.default_next = replace_map[state.default_next];
    tmp_set.clear();
    for (std::set<state_t>::iterator iter = state.dst_states.begin(); iter != state.dst_states.end(); ++iter) {
      if (*iter != REJECT) tmp_set.insert(replace_map[*iter]);
      else tmp_set.insert(REJECT);
    }
    state.dst_states = tmp_set;
    tmp_set.clear();
    for (std::set<state_t>::iterator iter = state.src_states.begin(); iter != state.src_states.end(); ++iter) {
      tmp_set.insert(replace_map[*iter]);
    }
    state.src_states = tmp_set;
  }

  transition_.resize(minimum_size);
  states_.resize(minimum_size);

  minimum_ = true;
  
  return;
}

void
DFA::Complement()
{
  state_t reject = REJECT;
  for (iterator state_iter = begin(); state_iter != end(); ++state_iter) {
    State &state = *state_iter;
    state.accept = !state.accept;
    bool to_reject = false;
    for (std::size_t i = 0; i < 256; i++) {
      if (state[i] == REJECT) {
        if (reject == REJECT) {
          State &reject_state = get_new_state();
          reject = reject_state.id;
          reject_state.dst_states.insert(reject);
          reject_state.accept = true;
          to_reject = true;          
        }
        state[i] = reject;
      }
    }
    if (state.default_next == REJECT) {
      state.default_next = reject;
    }
    if (to_reject) {
      state.dst_states.insert(reject);
      states_[reject].src_states.insert(state.id);
    }
  }
}

#if REGEN_ENABLE_XBYAK
class XbyakCompiler: public Xbyak::CodeGenerator {
 public:
  XbyakCompiler(const DFA &dfa, std::size_t state_code_size);
 private:
  std::size_t code_segment_size_;
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

XbyakCompiler::XbyakCompiler(const DFA &dfa, std::size_t state_code_size = 64):
    /* code segment for state transition.
     *   each states code was 16byte alligned.
     *                        ~~
     *   padding for 4kb allign between code and data
     *                        ~~
     * data segment for transition table
     *                                                */
    CodeGenerator(code_segment_size(dfa.size()) + data_segment_size(dfa.size())),
    code_segment_size_(code_segment_size(dfa.size()))
{
  std::vector<const uint8_t*> states_addr(dfa.size());

  const uint8_t* code_addr_top = getCurr();
  const uint8_t** transition_table_ptr = (const uint8_t **)(code_addr_top + code_segment_size_);

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
    dfa.state2label(i, labelbuf);
    L(labelbuf);
    states_addr[i] = getCurr();
    // can transition without table lookup ?
    
    const DFA::AlterTrans &at = dfa[i].alter_transition;
    if (dfa.olevel() >= O2 && at.next1 != DFA::NONE) {
      std::size_t state = i;
      std::size_t inline_level = dfa.olevel() == O3 ? dfa[i].inline_level : 0;
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
      assert(at.next1 != DFA::NONE);
      dfa.state2label(at.next1, labelbuf);
      if (at.next1 != DFA::REJECT) {
        jn_flag = true;  
      }
      if (at.next2 == DFA::NONE) {
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
        dfa.state2label(at.next2, labelbuf);
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
      DFA::state_t next = trans[c];
      transition_table_ptr[i*256+c] =
          (next  == DFA::REJECT ? reject_state_addr : states_addr[next]);
    }
  }
}

bool DFA::EliminateBranch()
{
  for (iterator state_iter = begin(); state_iter != end(); ++state_iter) {
    State &state = *state_iter;
    state_t next1 = state[0], next2 = DFA::NONE;
    unsigned int begin = 0, end = 256;
    int c;
    for (c = 1; c < 256 && next1 == state[c]; c++);
    if (c < 256) {
      next2 = next1;
      next1 = state[c];
      begin = c;
      for (++c; c < 256 && next1 == state[c]; c++);
    }
    if (c < 256) {
      end = --c;
      for (++c; c < 256 && next2 == state[c]; c++);
    }
    if (c < 256) {
      next1 = next2 = DFA::NONE;
    }
    state.alter_transition.next1 = next1;
    state.alter_transition.next2 = next2;
    state.alter_transition.key.first = begin;
    state.alter_transition.key.second = end;
  }

  return true;
}

bool DFA::Reduce()
{
  states_[0].src_states.insert(DFA::NONE);
  std::vector<bool> inlined(size());

  for (iterator state_iter = begin(); state_iter != end(); ++state_iter) {
    // Pick inlining region (make degenerate graph).
    state_t state_id = state_iter->id;
    if (inlined[state_id]) continue;
    state_t current_id = state_id;
    for(;;) {
      State &current = states_[current_id];
      if (current.dst_states.size() > 2 ||
          current.dst_states.size() == 0) break;
      if (current.dst_states.size() == 2 &&
          current.dst_states.count(DFA::REJECT) == 0) break;
      if (current.dst_states.size() == 1 &&
          current.dst_states.count(DFA::REJECT) == 1) break;
      State &next = states_[*(current.dst_states.lower_bound(0))];
      if (next.alter_transition.next1 == DFA::NONE) break;
      if (next.src_states.size() != 1 ||
          next.accept) break;
      if (inlined[next.id]) break;
      inlined[next.id] = true;
      current_id = next.id;

      if(++(state_iter->inline_level) >= 10) break;
    }
  }
  states_[0].src_states.erase(DFA::NONE);

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
  CompiledFullMatch = (state_t (*)(const unsigned char *, const unsigned char *))xgen_->getCode();
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
  state_t state = 0;

  if (olevel_ >= O1) {
    state = CompiledFullMatch(str, end);
    return state != DFA::REJECT ? states_[state].accept : false;
  }

  while (str < end && (state = transition_[state][*str++]) != DFA::REJECT);

  return state != DFA::REJECT ? states_[state].accept : false;
}

} // namespace regen
