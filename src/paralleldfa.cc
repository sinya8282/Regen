#include "paralleldfa.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace regen {

ParallelDFA::ParallelDFA(const DFA &dfa, std::size_t thread_num):
    dfa_size_(dfa.size()),
    thread_num_(thread_num)
{
  dfa_accepts_ = dfa.accepts();

  ParallelTransition parallel_transition;
  std::map<ParallelTransition, int> pdfa_map;
  std::queue<ParallelTransition> queue;
  ParallelTransition::iterator iter;
  std::size_t pdfa_id = 0;

  pdfa_map[parallel_transition] = DFA::REJECT;

  for (std::size_t i = 0; i < dfa_size_; i++) {
    parallel_transition[i] = i;
  }

  pdfa_map[parallel_transition] = pdfa_id++;
  queue.push(parallel_transition);

  while (!queue.empty()) {
    parallel_transition = queue.front();
    parallel_transitions_.push_back(parallel_transition);
    queue.pop();
    std::vector<ParallelTransition> transition(256);
    bool is_accept = parallel_transition.size() == dfa_size_;

     iter = parallel_transition.begin();
      while (iter != parallel_transition.end()) {
        int start = (*iter).first;
        int current = (*iter).second;
        const Transition &trans = dfa.GetTransition(current);
        for (std::size_t c = 0; c < 256; c++) {
          int next = trans[c];
          if (next != DFA::REJECT) {
            transition[c][start] = next;
          }
        }
        if (!dfa_accepts_[current]) is_accept = false;
        ++iter;
     }

    DFA::Transition &dfa_transition = get_new_transition();
    std::set<int> dst_state;
    
    for (std::size_t c = 0; c < 256; c++) {
      ParallelTransition &next = transition[c];
      if (next.empty()) continue;

      if (pdfa_map.find(next) == pdfa_map.end()) {
        pdfa_map[next] = pdfa_id++;
        queue.push(next);
      }
      dfa_transition[c] = pdfa_map[next];
      dst_state.insert(pdfa_map[next]);
    }
    set_state_info(is_accept ,DFA::REJECT, dst_state);
  }

  transition__ = (int*)malloc(sizeof(int)*256*transition_.size());
  for (std::size_t s = 0; s < transition_.size(); s++) {
    for (std::size_t c = 0; c < 256; c++) {
      transition__[s*256+c] = transition_[s][c];
    }
  }
}

#if 1
class XbyakCompiler: public Xbyak::CodeGenerator {
 public:
  XbyakCompiler(const ParallelDFA &pdfa, std::size_t state_code_size);
};

XbyakCompiler::XbyakCompiler(const ParallelDFA &pdfa, std::size_t state_code_size = 32):
    /* pdfa.size()*32 for code. <- code segment
     * padding for 4kb allign 
     * pdfa.size()*256*sizeof(void *) for transition table. <- data segment */
    CodeGenerator(  (pdfa.size()+3)*state_code_size
                  + ((pdfa.size()+3)*state_code_size % 4096)
                  + pdfa.size()*256*sizeof(void *))
{
  std::vector<const uint8_t*> states_addr(pdfa.size());

  const uint8_t* code_addr_top = getCurr();
  const uint8_t** transition_table_ptr = (const uint8_t **)(code_addr_top + (pdfa.size()+3)*state_code_size + (((pdfa.size()+3)*state_code_size) % 4096));

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

  L("accept");
  ret();

  L("reject");
  const uint8_t *reject_state_addr = getCurr();
  mov(rax, -1); // return reject state number
  ret();

  align(32);
  
  // state code generation, and indexing every states address.
  L("s0");
  char labelbuf[100];
  for (std::size_t i = 0; i < pdfa.size(); i++) {
    sprintf(labelbuf, "accept%d", i);
    states_addr[i] = getCurr();
    cmp(arg1, arg2);
    je(labelbuf);
    movzx(tmp1, byte[arg1]);
    inc(arg1);
    jmp(ptr[tbl+i*256*8+tmp1*8]);
    L(labelbuf);
    mov(rax, i); // embedding state number (for debug).
    je("accept");

    align(32);
  }
  
  // backpatching (every states address)
  for (std::size_t i = 0; i < pdfa.size(); i++) {
    const DFA::Transition &trans = pdfa.GetTransition(i);
    for (int c = 0; c < 256; c++) {
      int next = trans[c];
      transition_table_ptr[i*256+c] =
          (next  == -1 ? reject_state_addr : states_addr[next]);
    }
  }
}

bool ParallelDFA::Compile()
{
  xgen_ = new XbyakCompiler(*this);
  CompiledFullMatchTask = (int (*)(const unsigned char *, const unsigned char *))xgen_->getCode();
  return true;
}
#else
bool ParallelDFA::Compile()
{
  return true;
}
#endif

void
ParallelDFA::FullMatchTask(TaskArg targ) const
{
  const unsigned char *str = targ.str;
  const unsigned char *end = targ.end;

  if (CompiledFullMatchTask != NULL) {
    parallel_states_[targ.task_id] = CompiledFullMatchTask(str, end);
    return;
  }
  
  int state = 0;

  while (str != end && (state = transition_[state][(*str++)]) != DFA::REJECT);

  parallel_states_[targ.task_id] = state;
  return;
}

bool
ParallelDFA::FullMatch(const unsigned char *str, const unsigned char *end) const
{
  parallel_states_.resize(thread_num_);
  boost::thread *threads[thread_num_];
  std::size_t task_string_length = (end-str) / thread_num_;
  std::size_t remainder_length = (end-str) % thread_num_;
  const unsigned char *str_ = str, *end_;
  TaskArg targ;

  for (std::size_t i = 0; i < thread_num_; i++) {
    end_ = str_ + task_string_length;
    if (i == thread_num_ - 1) end_ += remainder_length;
    targ.str = str_;
    targ.end = end_;
    targ.task_id = i;
    threads[i] = new boost::thread(
        boost::bind(
            boost::bind(&regen::ParallelDFA::FullMatchTask, this, _1),
            targ));
    str_ = end_;
  }

  int dstate = 0, pstate;
  bool match = false;
  for (std::size_t i = 0; i < thread_num_; i++) {
    threads[i]->join();
    if ((pstate = parallel_states_[i]) == DFA::REJECT) break;
    ParallelTransition::const_iterator iter = parallel_transitions_[pstate].find(dstate);
    if (iter == parallel_transitions_[pstate].end()) break;
    dstate = (*iter).second;
  }

  if (dstate != DFA::REJECT) {
    match = dfa_accepts_[dstate];
  }

  for (std::size_t i = 0; i < thread_num_; i++) {
    delete threads[i];
  }
  
  return match;
}



} // namespace regen
