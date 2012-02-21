#include "jitter.h"
#include "dfa.h"

namespace regen {

CodeSegment::CodeSegment(const DFA &dfa, Jitter &jitter, std::size_t code_segment_size):
    CodeGenerator(code_segment_size),
    dfa_(dfa), jitter_(jitter), code_segment_size_(code_segment_size),
    reject_addr_(NULL), return_addr_(NULL),
#ifdef XBYAK32
    arg1(ecx), arg2(edx), arg3(ebx),
    tbl (ebp), tmp1(esi), tmp2(edi), reg_a(eax)
#elif defined(XBYAK64_WIN)
    arg1(rcx), arg2(rdx), arg3(r8),
    tbl (r9), tmp1(r10), tmp2(r11), reg_a(rax),
#else
    arg1(rdi), arg2(rsi), arg3(rdx),
    tbl (r8), tmp1(r10), tmp2(r11), reg_a(rax)
#endif
{
}

const void* CodeSegment::EmitFunc()
{
  const void* func_ptr = getCurr();
#ifdef XBYAK32
  push(esi);
  push(edi);
  push(ebp);
  push(ebx);
  const int P_ = 4 * 4;
  mov(arg1, ptr [esp + P_ + 4]);
  mov(arg2, ptr [esp + P_ + 8]);
  mov(arg3, ptr [esp + P_ + 12]);
#endif
  mov(tmp2, 0);
  jmp("s0");
  L("reject");
  reject_addr_ = getCurr();
  mov(reg_a, DFA::REJECT); // return false
  L("return");
  return_addr_ = getCurr();
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

  return func_ptr;
}

const void* CodeSegment::EmitState(std::size_t state)
{
  const void* state_ptr = getCurr();
  return state_ptr;
}

void Jitter::Init()
{
  NewCS();
  func_ptr_ = CS()->EmitFunc();
}

} // namespace regen
