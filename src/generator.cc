#include "generator.h"
#include <xbyak/xbyak.h>

namespace regen {

namespace Generator {

char* normalize(int c, char *buf)
{
  int index = 0;
  if (' ' <= c && c <= '~') {
    if (c == '"') {
        buf[index++] = '\\';
    }
    buf[index++] = c;
    buf[index] = '\0';
  } else {
    sprintf(buf, "\\x%02x", c);
  }
  return buf;
}

void DotGenerate(const Regex &regex)
{
  static const char* const normal = "circle";
  static const char* const accept = "doublecircle";
  static const char* const thema  = "fillcolor=lightsteelblue1, style=filled, color = navyblue ";
  char buf[10];
  puts("digraph DFA {\n  rankdir=\"LR\"");
  const DFA &dfa = regex.dfa();
  for (std::size_t i = 0; i < dfa.size(); i++) {
    printf("  q%"PRIuS" [shape=%s, %s]\n", i, (dfa.IsAcceptState(i) ? accept : normal), thema);
  }
  printf("  start [shape=point]\n  start -> q0\n\n");
  for (std::size_t state = 0; state < dfa.size(); state++) {
    const DFA::Transition &transition = dfa.GetTransition(state);

    for (int input = 0; input < 256; input++) {
      if (transition[input] != DFA::REJECT &&
          transition[input] != dfa.GetDefaultNext(state)) {
        printf("  q%"PRIuS" -> q%d [label=\"", state, transition[input]);
        if (input < 255 && transition[input] == transition[input+1]) {
          printf("[%s", normalize(input, buf));
          while (++input < 255) {
            if (transition[input] != transition[input+1]) break;
          }
          printf("-%s]", normalize(input, buf));
        } else {
          printf("%s", normalize(input, buf));
        }
        printf("\"]\n");
      }
    }
    
    if (dfa.GetDefaultNext(state) != DFA::REJECT) {
      printf("  q%"PRIuS" -> q%d [color=red]\n",
             state, dfa.GetDefaultNext(state));
    }

  }
  puts("}");
}

void CGenerate(const Regex& regex)
{
  puts("typedef unsigned char  UCHAR;");
  puts("typedef unsigned char *UCHARP;");
  const DFA &dfa = regex.dfa();
  for (std::size_t i = 0; i < dfa.size(); i++) {
    const DFA::Transition &transition = dfa.GetTransition(i);
    printf("void s%"PRIuS"(UCHARP beg, UCHARP buf, UCHARP end)\n{\n", i);
    if (dfa.IsAcceptState(i)) {
      puts("  return accept(beg, buf, end);");
    } else {
      printf("  static void (*table[256])(UCHARP, UCHARP, UCHARP) = { \n  ");
      for (int j = 0; j < 256; j++) {
        if (transition[j] == DFA::REJECT) {
          printf("reject");
        } else {
          printf("s%d", transition[j]);
        }
        if (j < 255) printf(", ");
      }
      puts("  };");
      puts("  return table[*buf++](beg, buf, end);");
    }
    puts("}");
  }  
}

class XbyakGenerator: public Xbyak::CodeGenerator {
 public:
  XbyakGenerator(const DFA &dfa, std::size_t state_code_size);
 private:
  const DFA dfa_;
};

XbyakGenerator::XbyakGenerator(const DFA &dfa, std::size_t state_code_size = 32):
    /* dfa.size()*32 for code. <- code segment
     * padding for 4kb allign 
     * dfa.size()*256*sizeof(void *) for transition table. <- data segment */
    CodeGenerator(  (dfa.size()+3)*state_code_size
                  + ((dfa.size()+3)*state_code_size % 4096)
                  + dfa.size()*256*sizeof(void *))
{
  std::vector<const uint8_t*> states_addr(dfa.size());

  const uint8_t* code_addr_top = getCurr();
  const uint8_t** transition_table_ptr = (const uint8_t **)(code_addr_top + (dfa.size()+3)*state_code_size + (((dfa.size()+3)*state_code_size) % 4096));

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
  mov(rax, 1); // return true
  ret();

  L("reject");
  const uint8_t *reject_state_addr = getCurr();
  xor(rax, rax); // return false
  ret();

  align(32);
  
  // state code generation, and indexing every states address.
  L("s0");
  for (std::size_t i = 0; i < dfa.size(); i++) {
    states_addr[i] = getCurr();
    cmp(arg1, arg2);
    if (dfa.IsAcceptState(i)) {
      je("accept", T_NEAR);
    } else {
      je("reject", T_NEAR);
    }
    movzx(tmp1, byte[arg1]);
    inc(arg1);
    jmp(ptr[tbl+i*256*8+tmp1*8]);
    mov(tmp2, i); // embedding state number (for debug).

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

void XbyakGenerate(const Regex &regex)
{
  const DFA &dfa = regex.dfa();
  XbyakGenerator xgen(dfa);
  bool (*dfa_)(const char *, const char *) = (bool (*)(const char *, const char *))xgen.getCode();

  Util::mmap_t mm("hoge");
  bool result = dfa_((char *)mm.ptr, (char *)mm.ptr+mm.size);
  printf("%d\n", result);

  return;
}

} // namespace Generator

} // namespace regen
