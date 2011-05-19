#include "regex.h"
#include "exprutil.h"
#include "dfa.h"
#include "generator.h"

int main(int argc, char *argv[]) {
  std::string str;
  std::ifstream ifs(argv[1]);
  ifs >> str;
  regen::Regex r = regen::Regex(str);
  regen::Generator::DotGenerate(r);
  return 0;
}

