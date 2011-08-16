#include "../regex.h"
#include "../paralleldfa.h"

int main(int argc, char *argv[]) {
  std::string regex;
  int opt;

  while ((opt = getopt(argc, argv, "f:")) != -1) {
    switch(opt) {
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
    }
  }
  
  if (regex.empty()) {
    if (optind >= argc) {
      exitmsg("USAGE: regen [options] regexp\n");
    } else {
      regex = std::string(argv[optind]);
    }
  }

  regen::Regex r = regen::Regex(regex);
  r.Compile(regen::O0);  
  regen::ParallelDFA pdfaD(r.dfa());

  printf("NFA state num:  %"PRIuS"\nDFA state num: %"PRIuS"\nPDFA(from DFA) state num: %"PRIuS"\n",
         r.state_exprs().size(), r.dfa().size(), pdfaD.size());

  return 0;
}
