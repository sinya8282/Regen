#include "regen.h"
#include "regex.h"
#include "exprutil.h"
#include "dfa.h"
#include "generator.h"

int main(int argc, char *argv[]) {
  enum Generate { DOTGEN, REGEN, CGEN};
  std::string regex;
  bool minimize = false;
  int opt;
  Generate generate = REGEN;

  while ((opt = getopt(argc, argv, "mdcxef:")) != -1) {
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
      case 'm': {
        minimize = true;
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

  regen::Regex r = regen::Regex(regex, Regen::Options::Extended);
  if (generate == REGEN && !minimize) {
    r.PrintRegex();
    return 0;
  } else {
    r.Compile(Regen::Options::O0);
    if (minimize) r.MinimizeDFA();
  }    

  switch (generate) {
    case CGEN:
      regen::Generator::CGenerate(r);
      break;
    case DOTGEN:
      r.MinimizeDFA();
      regen::Generator::DotGenerate(r);
      break;
    case REGEN: break; // Unreachable
  }
  return 0;
}
