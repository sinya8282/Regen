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
    if (optind+1 >= argc) {
      exitmsg("USAGE: regen [-c] [-t thread_num] regexp file\n");
    } else {
      regex = std::string(argv[optind++]);
    }
  }

  regen::Util::mmap_t mm(argv[optind]);
  regen::Regex r = regen::Regex(regex);

  if (thread_num <= 1) {
    if (compile) r.Compile();
    puts(r.FullMatch(mm.ptr, mm.ptr+mm.size) ? "match." : "not match.");
  } else {
    const regen::DFA &dfa = r.dfa();
    regen::ParallelDFA pdfa = regen::ParallelDFA(dfa, thread_num);
    if (compile) pdfa.Compile();
    puts(pdfa.FullMatch(mm.ptr, mm.ptr+mm.size) ? "match." : "not match.");
  }

  return 0;
}
