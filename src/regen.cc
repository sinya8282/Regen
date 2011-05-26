#include "regex.h"
#include "exprutil.h"
#include "dfa.h"
#include "generator.h"

int main(int argc, char *argv[]) {
  enum Generate { CGEN, DOTGEN, XGEN, EVAL};
  std::string regex;
  int opt;
  Generate generate = CGEN;

  while ((opt = getopt(argc, argv, "dcxef:")) != -1) {
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
      case 'x': {
        generate = XGEN;
        break;
      }
      case 'e': {
        generate = EVAL;
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

  regen::Regex r = regen::Regex(regex);

  switch (generate) {
    case CGEN:
      regen::Generator::CGenerate(r);
      break;
    case DOTGEN:
      regen::Generator::DotGenerate(r);
      break;
    case XGEN:
      // JIT compile, and evalute that.
      regen::Generator::XbyakGenerate(r);
      break;
    case EVAL:
      for (;;) {
        std::string input;
        std::cin >> input;
        if (r.IsMatched(input)) {
          std::cout << input << std::endl;
        }
      }
      break;
  }
  return 0;
}

