#include "dfa.h"

namespace regen {

DFA::DFA(const ExprInfo &expr_info, std::size_t limit):
    expr_info_(expr_info), complete_(false), minimum_(false), olevel_(Regen::Options::O0)
#ifdef REGEN_ENABLE_XBYAK
    , xgen_(NULL)
#endif
{
  complete_ = Construct(limit);
}

DFA::DFA(const NFA &nfa, std::size_t limit):
    complete_(false), minimum_(false), olevel_(Regen::Options::O0)
#ifdef REGEN_ENABLE_XBYAK
    , xgen_(NULL)
#endif
{
  complete_ = Construct(nfa, limit);
}

bool DFA::Construct(std::size_t limit)
{
  if (expr_info_.expr_root == NULL) return false;
  typedef std::set<StateExpr*> NFA;
  std::queue<NFA> queue;

  state_t dfa_id = 0;
  bool limit_over = false;

  nfa_map_[0] = expr_info_.expr_root->transition().first;
  dfa_map_[expr_info_.expr_root->transition().first] = dfa_id++;

  queue.push(expr_info_.expr_root->transition().first);

  std::vector<NFA> transition(256);
  std::set<Operator*> intersects;
  std::map<std::size_t, Operator*> exclusives;
  NFA next_non_greedy;
  
  while (!queue.empty()) {
    NFA nfa_states = queue.front();
    queue.pop();
    std::fill(transition.begin(), transition.end(), NFA());
    intersects.clear();
    exclusives.clear();
    next_non_greedy.clear();
    bool is_accept = false;

pretransition:
    for (NFA::iterator iter = nfa_states.begin(); iter != nfa_states.end(); ++iter) {
      switch ((*iter)->type()) {
        case Expr::kOperator: {
          Operator *op = static_cast<Operator*>(*iter);
          switch (op->optype()) {
            case Operator::kIntersection:
              if (op->pair()->active()) {
                std::size_t presize = nfa_states.size();
                nfa_states.insert(op->transition().follow.begin(),
                                  op->transition().follow.end());
                if (presize < nfa_states.size()) {
                  iter = nfa_states.begin();
                  continue;
                }
              } else {
                op->set_active(true);
                intersects.insert(op);
              }
              break;
            case Operator::kXOR:
              op->set_active(true);
              exclusives[op->id()] = op;
              break;
            default:
              break;
          }
        }
          break;
        case Expr::kEOP:
          is_accept = true;
          break;
        default:
          break;
      }
    }
    std::size_t presize = nfa_states.size();
    for (std::map<std::size_t, Operator*>::iterator iter = exclusives.begin();
         iter != exclusives.end(); ++iter) {
      Operator *op = static_cast<Operator *>(iter->second);
      if (!(op->pair()->active())) {
        nfa_states.insert(op->transition().follow.begin(),
                          op->transition().follow.end());
      }
      if (presize < nfa_states.size()) goto pretransition;
    }

    for (NFA::iterator iter = nfa_states.begin(); iter != nfa_states.end(); ++iter) {
      if ((*iter)->non_greedy()) {
        if (!is_accept) next_non_greedy.insert(*iter);
        continue;
      }
      NFA next = (*iter)->transition().follow;
      (*iter)->FillTransition(transition);
    }

    for (std::set<Operator*>::iterator iter = intersects.begin();
         iter != intersects.end(); ++iter) {
      (*iter)->set_active(false);
    }
    for (std::map<std::size_t, Operator*>::iterator iter = exclusives.begin();
         iter != exclusives.end(); ++iter) {
      iter->second->set_active(false);
      iter->second->pair()->set_active(false);
    }
    
    State &state = get_new_state();
    Transition &trans = transition_[state.id];
    state.accept = is_accept;
    //only Leftmost-Shortest matching
    if (flag_.shortest_match() && is_accept) {
      trans.fill(REJECT);
      state.dst_states.insert(REJECT);
      continue;
    }

    for (state_t i = 0; i < 256; i++) {
      NFA &next = transition[i];
      bool to_accept = std::find(next.begin(), next.end(), expr_info_.eop) != next.end();
      if (!to_accept && !next_non_greedy.empty()) {
        for(NFA::iterator iter = next_non_greedy.begin(); iter != next_non_greedy.end(); ++iter) {
          if ((*iter)->Match(i)) next.insert((*iter)->transition().follow.begin(), (*iter)->transition().follow.end());
        }
      }
      if (next.empty()) {
        trans[i] = REJECT;
        state.dst_states.insert(REJECT);
        continue;
      }
      if (dfa_map_.find(next) == dfa_map_.end()) {
        if (dfa_id < limit) {
          nfa_map_[dfa_id] = next;
          dfa_map_[next] = dfa_id++;
          queue.push(next);          
        } else {
          limit_over = true;
          continue;
        }
      }
      trans[i] = dfa_map_[next];
      state.dst_states.insert(dfa_map_[next]);
    }
  }

  if (limit_over) {
    return false;
  } else {
    Finalize();
    return true;
  }
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
    state.accept = is_accept;
    //only Leftmost-Shortest matching
    if (flag_.shortest_match() && is_accept) {
      trans.fill(REJECT);
      continue;
    }
    
    for (state_t i = 0; i < 256; i++) {
      NFA_ &next = transition[i];
      if (next.empty()) {
        trans[i] = REJECT;
        state.dst_states.insert(REJECT);
        continue;
      }
      if (dfa_map.find(next) == dfa_map.end()) {
        dfa_map[next] = dfa_id++;
        queue.push(next);
      }
      trans[i] = dfa_map[next];
      state.dst_states.insert(dfa_map[next]);
    }
  }

  Finalize();

  return true;
}

DFA::state_t DFA::OnTheFlyConstructWithChar(state_t state, unsigned char input, Regen::Context *context) const
{
  typedef std::set<StateExpr*> NFA_;
  NFA_::iterator iter, next_iter;
  NFA_ &nfa = nfa_map_[state], next;
  bool accept = false;

  for (iter = nfa.begin(); iter != nfa.end(); ++iter) {
    StateExpr *s = *iter;
    if (s->Match(input)) {
      for (next_iter = s->transition().follow.begin();
           next_iter != s->transition().follow.end();
           ++next_iter) {
        next.insert(*next_iter);
        if ((*next_iter)->type() == Expr::kEOP) accept = true;
      }
    }
  }

  if (next.empty()) {
    transition_[state][input] = REJECT;
    return REJECT;
  }

  if (dfa_map_.find(next) == dfa_map_.end()) {
    State& s = get_new_state();
    dfa_map_[next] = s.id;
    nfa_map_[s.id] = next;
    s.accept = accept;
    transition_[state][input] = s.id;
    return s.id;
  }

  transition_[state][input] = dfa_map_[next];
  return dfa_map_[next];
}

std::pair<DFA::state_t, const unsigned char *> DFA::OnTheFlyConstructWithString(state_t state, const unsigned char *begin, const unsigned char *end, Regen::Context *context) const
{
  state_t next;
  const unsigned char *ptr;
  for (ptr = begin; ptr < end; ptr++) {
    if (transition_[state][*ptr] == UNDEF) {
      next = OnTheFlyConstructWithChar(state, *ptr, context);
      if (next == REJECT) break;
      state = next;
    } else {
      break;
    }
  }

  std::pair<state_t, const unsigned char *> ret(state, ptr);
  return ret;
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

  complete_ = true;
}

DFA::State& DFA::get_new_state() const {
  transition_.resize(states_.size()+1);
  states_.resize(states_.size()+1);
  State &new_state = states_.back();
  new_state.transitions = &transition_;
  new_state.id = states_.size()-1;
  new_state.alter_transition.next1 = UNDEF;
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

bool DFA::Minimize()
{
  if (!complete_) return false;
  if (minimum_) return true;
  
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
    return true;
  }

  size_t minimum_size = size() - swap_map.size();
  std::vector<state_t> replace_map(size());
  for (state_t s = 0, d = 0; s < size(); s++) {
    if (swap_map.find(s) == swap_map.end()) {
      replace_map[s] = d++;
      if (s != replace_map[s]) {
        transition_[replace_map[s]] = transition_[s];
        states_[replace_map[s]] = states_[s];
        states_[replace_map[s]].id = replace_map[s];
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
  return true;
}

void DFA::Complementify()
{
  state_t reject = REJECT;
  for (iterator state_iter = begin(); state_iter != end(); ++state_iter) {
    State &state = *state_iter;
    if (state.id != reject) {
      state.accept = !state.accept;
    }
    bool to_reject = false;
    for (std::size_t i = 0; i < 256; i++) {
      if (state[i] == REJECT) {
        if (reject == REJECT) {
          State &reject_state = get_new_state();
          reject = reject_state.id;
          for (std::size_t j = 0; j < 256; j++) {
            reject_state[j] = reject;
          }
          reject_state.dst_states.insert(reject);
          reject_state.accept = true;
        }
        to_reject = true;
        state[i] = reject;
      }
    }
    if (to_reject) {
      state.dst_states.insert(reject);
      states_[reject].src_states.insert(state.id);
    }
  }
}

#if REGEN_ENABLE_XBYAK
JITCompiler::JITCompiler(const DFA &dfa, std::size_t state_code_size = 64):
    /* code segment for state transition.
     *   each states code was 16byte alligned.
     *                        ~~
     *   padding for 4kb allign between code and data
     *                        ~~
     * data segment for transition table
     *                                                */
    CodeGenerator(code_segment_size(dfa.size()) + data_segment_size(dfa.size())),
    code_segment_size_(code_segment_size(dfa.size())),
    data_segment_size_(data_segment_size(dfa.size())),
    total_segment_size_(code_segment_size(dfa.size())+data_segment_size(dfa.size()))
{
  std::vector<const uint8_t*> states_addr(dfa.size());

  const uint8_t* code_addr_top = getCurr();
  const uint8_t** transition_table_ptr = (const uint8_t **)(code_addr_top + code_segment_size_);

#ifdef XBYAK32
  const Xbyak::Reg32& arg1(ecx);
  const Xbyak::Reg32& arg2(edx);
  const Xbyak::Reg32& arg3(ebx);
  const Xbyak::Reg32& tbl (ebp);
  const Xbyak::Reg32& tmp1(esi);
  const Xbyak::Reg32& tmp2(edi);  
  const Xbyak::Reg32& reg_a(eax);
  push(esi);
  push(edi);
  push(ebp);
  push(ebx);
  const int P_ = 4 * 4;
  mov(arg1, ptr [esp + P_ + 4]);
  mov(arg2, ptr [esp + P_ + 8]);
  mov(arg3, ptr [esp + P_ + 12]);
#elif defined(XBYAK64_WIN)
  const Xbyak::Reg64& arg1(rcx);
  const Xbyak::Reg64& arg2(rdx);
  const Xbyak::Reg64& arg3(r8);
  const Xbyak::Reg64& tbl (r9);
  const Xbyak::Reg64& tmp1(r10);
  const Xbyak::Reg64& tmp2(r11);
  const Xbyak::Reg64& reg_a(rax);
#else
  const Xbyak::Reg64& arg1(rdi);
  const Xbyak::Reg64& arg2(rsi);
  const Xbyak::Reg64& arg3(rdx);
  const Xbyak::Reg64& tbl (r8);
  const Xbyak::Reg64& tmp1(r10);
  const Xbyak::Reg64& tmp2(r11);
  const Xbyak::Reg64& reg_a(rax);
#endif

  // setup enviroment on register
  mov(tbl, (size_t)transition_table_ptr);
  mov(tmp2, 0);
  jmp("s0");
  L("reject");
  const uint8_t *reject_state_addr = getCurr();
  mov(reg_a, DFA::REJECT); // return false
  L("return");
  cmp(arg3, 0);
  je("finalize");
  mov(ptr[arg3+sizeof(uint8_t*)], tmp2);
  L("finalize");
#ifdef XBYAK32
  pop(ebx);
  pop(ebp);
  pop(edi);
  pop(esi);
#endif
  ret();

  align(16);

  const int sign = dfa.flag().reverse_match() ? -1 : 1;
  
  char labelbuf[100];
  // state code generation, and indexing every states address.
  for (std::size_t i = 0; i < dfa.size(); i++) {
    dfa.state2label(i, labelbuf);
    L(labelbuf);
    states_addr[i] = getCurr();
    if (dfa.IsAcceptState(i) && !dfa.flag().suffix_match()) {
      inLocalLabel();
      cmp(arg3, 0);
      je(".ret");
      mov(tmp2, arg1);
      if (!dfa.flag().shortest_match()) jmp("@f");
      L(".ret");
      mov(reg_a, i);
      jmp("return");
      L("@@");
      outLocalLabel();
    }
    // can transition without table lookup ?
    const DFA::AlterTrans &at = dfa[i].alter_transition;
    if (dfa.olevel() >= Regen::Options::O2 && at.next1 != DFA::UNDEF) {
      std::size_t state = i;
      std::size_t inline_level = dfa.olevel() == Regen::Options::O3 ? dfa[i].inline_level : 0;
      bool inlining = inline_level != 0;
      std::size_t transition_depth = -1;
      inLocalLabel();
      if (inlining) {
        lea(tmp1, ptr[arg1 + inline_level * sign]);
        if (dfa.flag().reverse_match()) {
          cmp(arg2, tmp1);
        } else { 
          cmp(tmp1, arg2);
        }
        jge(".ret", T_NEAR);
      } else {
        cmp(arg1, arg2);
        je(".ret", T_NEAR);        
      }
   emit_transition:
      const DFA::AlterTrans &at = dfa.GetAlterTrans(state);
      bool jn_flag = false;
      transition_depth++;
      assert(at.next1 != DFA::UNDEF);
      dfa.state2label(at.next1, labelbuf);
      if (at.next1 != DFA::REJECT) {
        jn_flag = true;  
      }
      if (at.next2 == DFA::UNDEF) {
        if (!inlining) {
          add(arg1, sign);
          jmp(labelbuf, T_NEAR);
        } else if (transition_depth == inline_level) {
          add(arg1, transition_depth * sign);
          jmp(labelbuf, T_NEAR);
        } else {
          state = at.next1;
          goto emit_transition;
        }
      } else {
        if (!inlining) {
          movzx(tmp1, byte[arg1]);
          add(arg1, sign);
        } else if (transition_depth == inline_level) {
          movzx(tmp1, byte[arg1 + transition_depth * sign]);
          add(arg1, (transition_depth+1) * sign);
        } else {
          movzx(tmp1, byte[arg1 + transition_depth * sign]);
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
        add(arg1, sign);
        jmp(ptr[tbl+i*256*sizeof(uint8_t*)+tmp1*sizeof(uint8_t*)]);
        L("@@");
        mov(reg_a, i);
        jmp("return");
      } else {
        L(".ret");
        mov(reg_a, i);
        jmp("return");
      }
      outLocalLabel();
    } else {
      cmp(arg1, arg2);
      je("@f");
      movzx(tmp1, byte[arg1]);
      add(arg1, sign);
      jmp(ptr[tbl+i*256*sizeof(uint8_t*)+tmp1*sizeof(uint8_t*)]);
      L("@@");
      mov(reg_a, i);
      jmp("return");
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
    state_t next1 = state[0], next2 = DFA::UNDEF;
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
      next1 = next2 = DFA::UNDEF;
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
  states_[0].src_states.insert(DFA::UNDEF);
  std::vector<bool> inlined(size());
  const std::size_t MAX_REDUCE = 10;

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
      if (next.alter_transition.next1 == DFA::UNDEF) break;
      if (next.src_states.size() != 1 ||
          next.accept) break;
      if (inlined[next.id]) break;
      inlined[next.id] = true;
      current_id = next.id;

      if(++(state_iter->inline_level) >= MAX_REDUCE) break;
    }
  }
  states_[0].src_states.erase(DFA::UNDEF);

  return true;
}

bool DFA::Compile(Regen::Options::CompileFlag olevel)
{
  if (!complete_) return false;
  if (olevel <= olevel_) return true;
  if (olevel >= Regen::Options::O2) {
    if (EliminateBranch()) {
      olevel_ = Regen::Options::O2;
    }
    if (olevel == Regen::Options::O3 && Reduce()) {
      olevel_ = Regen::Options::O3;
    }
  }
  xgen_ = new JITCompiler(*this);
  CompiledMatch = (state_t (*)(const unsigned char *, const unsigned char *, Regen::Context*))xgen_->getCode();
  if (olevel_ < Regen::Options::O1) olevel_ = Regen::Options::O1;
  return olevel == olevel_;
}
#else
bool DFA::EliminateBranch() { return false; }
bool DFA::Reduce() { return false; }
bool DFA::Compile(Regen::Options::CompileFlag) { return false; }
#endif

bool DFA::Match(const unsigned char *str, const unsigned char *end, Regen::Context *context) const
{
  if (!complete_) return OnTheFlyMatch(str, end, context);

  state_t state = 0;

  if (olevel_ >= Regen::Options::O1) {
    state = CompiledMatch(str, end, context);
    if (context == NULL) {
      return IsAcceptState(state);
    } else {
      if (flag_.suffix_match() && IsAcceptState(state)) {
        context->set_end((const char *)end);
        return true;
      } else {
        return context->end() != NULL;
      }
    }
  }

  int dir = str < end ? 1 : -1;
  while (str != end && (state = transition_[state][*str]) != DFA::REJECT) {
    str += dir;
  }

  return state != DFA::REJECT ? states_[state].accept : false;
}

bool DFA::OnTheFlyMatch(const unsigned char *str, const unsigned char *end, Regen::Context *context) const
{
  state_t state = 0, next = UNDEF;
  if (empty()) {
    std::set<StateExpr*> &first_states = expr_info_.expr_root->transition().first;
    std::set<StateExpr*>::iterator iter;
    bool accept = false;
    for (iter = first_states.begin(); iter != first_states.end(); ++iter) {
      if ((*iter)->type() == Expr::kEOP) accept = true;
    }
    State& s = get_new_state();
    dfa_map_[first_states] = s.id;
    nfa_map_[s.id] = first_states;
    s.accept = accept;
  }

  while (str < end) {
    next = transition_[state][*str++];
    if (next >= UNDEF) {
      if (next == REJECT) return false;
      std::pair<state_t, const unsigned char *> config = OnTheFlyConstructWithString(state, str-1, end, context);
      if (config.second >= end) {
        state = config.first;
        break;
      } else if ((next = transition_[config.first][*(config.second)]) == DFA::REJECT) {
        return false;
      }
      str = config.second+1;
    }
    state = next;
  }

  return state != DFA::REJECT ? states_[state].accept : false;
}

} // namespace regen
