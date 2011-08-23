#include "../regex.h"
#include "../ssfa.h"

int main(int argc, char *argv[]) {
  std::string regex;
  int opt;
  bool n,d,s;
  n = d = s = false;

  while ((opt = getopt(argc, argv, "f:nds")) != -1) {
    switch(opt) {
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
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

  regen::Regex r = regen::Regex(regex);

  if (n) {
    printf("NFA state num:  %"PRIuS"\n", r.state_exprs().size());
  }
  if (d) {
    r.Compile(regen::O0);  
    printf("DFA state num: %"PRIuS"\n", r.dfa().size());
  }
  if (s) {
    r.Compile(regen::O0);  
    regen::SSFA pdfa(r.dfa());
    printf("PDFA(from DFA) state num: %"PRIuS"\n", pdfa.size());
  }

  return 0;
}
