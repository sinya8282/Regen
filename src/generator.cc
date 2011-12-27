#include "generator.h"

namespace regen {

namespace Generator {

const char* normalize(unsigned int c, char *buf)
{
  std::size_t index = 0;
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

void DotGenerate(const DFA &dfa)
{
  static const char* const normal = "circle";
  static const char* const accept = "doublecircle";
  static const char* const thema  = "fillcolor=lightsteelblue1, style=filled, color = navyblue ";
  char buf[16] = "";

  puts("digraph DFA {\n  rankdir=\"LR\"");
  for (std::size_t i = 0; i < dfa.size(); i++) {
    printf("  q%"PRIuS" [shape=%s, %s]\n", i, (dfa.IsAcceptState(i) ? accept : normal), thema);
  }
  printf("  start [shape=point]\n  start -> q0\n\n");
  for (std::size_t state = 0; state < dfa.size(); state++) {
    const DFA::Transition &transition = dfa.GetTransition(state);

    for (unsigned int input = 0; input < 256; input++) {
      if (transition[input] != DFA::REJECT) {
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
  }
  puts("}");
}

void CGenerate(const DFA &dfa)
{
  puts("typedef unsigned char  UCHAR;");
  puts("typedef unsigned char *UCHARP;");
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

} // namespace Generator

} // namespace regen
