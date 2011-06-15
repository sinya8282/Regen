#include "regex.h"
#include "exprutil.h"
#include "dfa.h"
#include "generator.h"

int main(int argc, char *argv[]) {
  enum Generate { DOTGEN, REGEN, CGEN};
  std::string regex;
  int opt;
  int recursive_depth = 2;
  Generate generate = REGEN;

  while ((opt = getopt(argc, argv, "dcxepf:r:")) != -1) {
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
       recursive_depth = atoi(optarg);
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

  regen::Regex r = regen::Regex(regex, recursive_depth);

  switch (generate) {
    case CGEN:
      regen::Generator::CGenerate(r);
      break;
    case DOTGEN:
      regen::Generator::DotGenerate(r);
      break;
    case REGEN:
      r.PrintRegex();
      break;
  }
  return 0;
}

