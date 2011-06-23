#include "regex.h"
#include "dfa.h"
#include "paralleldfa.h"

int main(int argc, char *argv[]) {
  std::string regex;
  int opt;
  int thread_num = 2;
  bool compile = false;

  while ((opt = getopt(argc, argv, "cf:t:")) != -1) {
    switch(opt) {
      case 'c':
        compile = true;
        break;
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
  if (compile) r.Compile();
  //regen::ParallelDFA pdfa = regen::ParallelDFA(dfa, thread_num);

  std::string text;
  regen::Util::mmap_t mm(argv[optind]);

  bool r1 = r.FullMatch(mm.ptr, mm.ptr+mm.size);
  printf("PDFA: %d\n", r1);
  
  return 0;
}
