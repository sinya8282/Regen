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
  regen::ParallelDFA pdfaN(r.expr_root(), r.state_exprs());
  r.Compile(regen::O0);  
  regen::ParallelDFA pdfaD(r.dfa());

  printf(" DFA state num: %"PRIuS"\n PDFA(from NFA) state num: %"PRIuS"\n PDFA(from DFA) state num: %"PRIuS"\n",
         r.dfa().size(), pdfaN.size(), pdfaD.size());

  return 0;
}
