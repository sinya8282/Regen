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
  r.Compile(regen::Regex::O0);
  regen::ParallelDFA pdfa(r.dfa());

  printf(" DFA state num: %d\n PDFA state num: %d\n", r.dfa().size(), pdfa.size());

  return 0;
}
