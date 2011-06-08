#include "regex.h"
#include "exprutil.h"
#include "dfa.h"
#include "generator.h"

int main(int argc, char *argv[]) {
  enum Generate { CGEN, DOTGEN, XGEN, EVAL, REGEN};
  std::string regex;
  int opt;
  int recursive_depth = 2;
  Generate generate = CGEN;

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
      case 'x': {
        generate = XGEN;
        break;
      }
      case 'e': {
        generate = EVAL;
        break;
      }
      case 'p': {
        generate = REGEN;
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
    case XGEN:
      // JIT compile, and evalute that.
      regen::Generator::XbyakGenerate(r);
      break;
    case REGEN:
      r.PrintRegex();
      break;
    case EVAL: {
      regen::Util::mmap_t mm("hoge");
      bool result = r.FullMatch((unsigned char *)mm.ptr, (unsigned char *)mm.ptr+mm.size);
      printf("%d\n", result);
      break;
      }
  }
  return 0;
}

