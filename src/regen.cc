#include "regex.h"
#include "exprutil.h"
#include "dfa.h"
#include "generator.h"

int main(int argc, char *argv[]) {
  std::string str;
  std::ifstream ifs(argv[1]);
  ifs >> str;
  regen::Regex* r = new regen::Regex(str);
  regen::DFA *dfa = r->CreateDFA();
  regen::Generator::DotGenerate(dfa);
  delete r;
  delete dfa;
  return 0;
}

