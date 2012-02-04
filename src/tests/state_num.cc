#include "../regen.h"
#include "../regex.h"
#include "../sfa.h"

int main(int argc, char *argv[]) {
  std::string regex;
  int opt;
  bool n,d,s,m,E;
  n = d = s = m = false;

  while ((opt = getopt(argc, argv, "Ef:ndsm")) != -1) {
    switch(opt) {
      case 'E':
        E = true;
        break;
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
      case 'm':
        m = true;
        break;
      case 'n':
        n = true;
        break;
      case 'd':
        d = true;
        break;
      case 's':
        s = true;
        break;
    }
  }

  if (!(n || d || s)) n = d = s = true;
  
  if (regex.empty()) {
    if (optind >= argc) {
      exitmsg("USAGE: regen [options] regexp\n");
    } else {
      regex = std::string(argv[optind]);
    }
  }

  Regen::Options option;
  option.extended(E);
  regen::Regex r = regen::Regex(regex, option);

  if (n) {
    printf("NFA state num:  %"PRIuS"\n", r.state_exprs().size());
  }
  if (d) {
    r.Compile(Regen::Options::O0);
    if (m) r.MinimizeDFA();
    printf("DFA state num: %"PRIuS"\n", r.dfa().size());
  }
  if (s) {
#ifdef REGEN_ENABLE_PARALLEL
    r.Compile(Regen::Options::O0);
    if (m) r.MinimizeDFA();
    regen::SFA sfa(r.dfa());
    printf("SFA(from DFA) state num: %"PRIuS"\n", sfa.size());
#else
    exitmsg("SFA is not supported.\n");
#endif
  }

  return 0;
}
