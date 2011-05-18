#include "generator.h"

namespace regen {

namespace Generator {

char* normalize(int c)
{
  static char buf[5];
  int index = 0;
  if (' ' <= c && c <= '~') {
    if (c == '"') {
        buf[index++] = '\\';
    }
    buf[index++] = c;
    buf[index] = '\0';
  } else {
    sprintf(buf, "0x%02x", c);
  }
  return buf;
}

void DotGenerate(DFA *dfa)
{
  static const char *normal = "circle";
  static const char *accept = "doublecircle";
  static const char *thema  = "fillcolor=lightsteelblue1, style=filled, color = navyblue ";
  puts("digraph DFA {\n  rankdir=LR\n");
  for (int i = 0; i < dfa->size(); i++) {
    printf("  q%d [shape=%s, %s]\n", i, (dfa->IsAcceptState(i) ? accept : normal), thema);
  }
  printf("  start [shape=point]\n  start -> q0\n\n");
  for (int state = 0; state < dfa->size(); state++) {
    const std::vector<int> &transition = dfa->GetTransition(state);

    for (int input = 0; input < 255; input++) {
      if (transition[input] != DFA::REJECT &&
          transition[input] != dfa->GetDefaultNext(state)) {
        printf("  q%d -> q%d [label=\"", state, transition[input]);
        if (input < 254 && transition[input] == transition[input+1]) {
          printf("[%s", normalize(input));
          while (++input < 254) {
            if (transition[input] != transition[input+1]) break;
          }
          printf("-%s]", normalize(input));
        } else {
          printf("%s", normalize(input));
        }
        printf("\"]\n");
      }
    }
    
    if (dfa->GetDefaultNext(state) != DFA::REJECT
        && !dfa->IsAcceptState(state)) {
      printf("  q%d -> q%d [color=red]\n",
             state, dfa->GetDefaultNext(state));
    }

  }
  puts("\n}");
}

}

} // namespace regen
