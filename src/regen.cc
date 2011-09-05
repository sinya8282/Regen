#include "regex.h"
#include "exprutil.h"
#include "dfa.h"
#include "generator.h"

int main(int argc, char *argv[]) {
  enum Generate { DOTGEN, REGEN, CGEN};
  std::string regex;
  int opt;
  int recursive_limit = 2;
  Generate generate = REGEN;

  while ((opt = getopt(argc, argv, "dcxef:r:")) != -1) {
    switch(opt) {
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
      case 'd': {
        generate = DOTGEN;
        break;
      }
      case 'c': {
        generate = CGEN;
        break;
      }
      case 'r': {
       recursive_limit = atoi(optarg);
       break;
      }
      default: exitmsg("USAGE: regen [options] regexp\n");
    }
  }
  
  if (regex.empty()) {
    if (optind >= argc) {
      exitmsg("USAGE: regen [options] regexp\n");
    } else {
      regex = std::string(argv[optind]);
    }
  }

  regen::Regex r = regen::Regex(regex, recursive_limit);
  r.Compile(regen::O0);

  switch (generate) {
    r.MinimizeDFA();
    case CGEN:
      regen::Generator::CGenerate(r);
      break;
    case DOTGEN:
      r.MinimizeDFA();      
      regen::Generator::DotGenerate(r);
      break;
    case REGEN:
      r.PrintRegex();
      break;
  }
  return 0;
}
