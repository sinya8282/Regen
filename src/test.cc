#include "regex.h"
#include "dfa.h"
#include "paralleldfa.h"
#include <boost/timer.hpp>

int main(int argc, char *argv[]) {
  std::string regex;
  int opt;
  int thread_num = 2;

  while ((opt = getopt(argc, argv, "f:t:")) != -1) {
    switch(opt) {
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
      case 't': {
        thread_num = atoi(optarg);
        break;
      }
    }
  }
  
  if (regex.empty()) {
    if (optind >= argc) {
      exitmsg("USAGE: regen [options] regexp\n");
    } else {
      regex = std::string(argv[optind++]);
    }
  }

  regen::Regex r = regen::Regex(regex);

  const regen::DFA &dfa = r.dfa();
  regen::ParallelDFA pdfa = regen::ParallelDFA(dfa, thread_num);

  std::string text;
  boost::timer t;
  regen::Util::mmap_t mm(argv[optind]);

  t.restart();
  bool r1 = pdfa.FullMatch(mm.ptr, mm.ptr+mm.size);
  printf("PDFA: %d(%.3f)\n", r1, t.elapsed());
  
  return 0;
}
