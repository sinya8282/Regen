#include "../regex.h"

struct testcase {
  testcase(std::string regex_, std::string text_, std::string pretty_, bool result_): regex(regex_), text(text_), pretty(pretty_), result(result_) {}
  std::string regex;
  std::string text;
  std::string pretty;
  bool result;
};

struct benchresult {
  uint64_t compile_time;
  uint64_t matching_time;
  bool result;
};

static inline uint64_t rdtsc()
{
  #ifdef _MSC_VER
  return __rdtsc();
  #else
  unsigned int eax, edx;
  __asm__ volatile("rdtsc" : "=a"(eax), "=d"(edx));
  return ((uint64_t)edx << 32) | eax;
  #endif
}

int main(int argc, char *argv[]) {
  int opt;
  regen::Optimize olevel = regen::Onone;

  while ((opt = getopt(argc, argv, "nf:t:O:")) != -1) {
    switch(opt) {
      case 'O': {
        olevel = regen::Optimize(atoi(optarg));
        break;
      }
    }
  }

  std::vector<testcase> bench;
  std::string text;
  for (std::size_t i = 0; i < 100; i++) {
    for (std::size_t j = 0; j < 10; j++) {
      text += "0123456789";
    }
    text += "_";
  }
  bench.push_back(testcase("((0123456789)_?)*", text, "((0123456789){10}_){100}", true));

  text = "a";
  for (std::size_t i = 0; i < 10; i++) {
    text += text;
  }

  bench.push_back(testcase("(a?){512}a{512}", text, "a{1024}", true));

  text += "bbbbbbbbbb";
  bench.push_back(testcase(".*b.{8}b", text, "a{1024}b{10}", true));

  /* fix..
  std::string regex = "http://((([a-zA-Z0-9]|[a-zA-Z0-9][-a-zA-Z0-9]*[a-zA-Z0-9])\\.)*([a-zA-Z]|[a-zA-Z][-a-zA-Z0-9]*[a-zA-Z0-9])\\.?|[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)(:[0-9]*)?(/([-_.!~*'()a-zA-Z0-9:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*(;([-_.!~*'()a-zA-Z0-9:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*)*(/([-_.!~*'()a-zA-Z0-9:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*(;([-_.!~*'()a-zA-Z0-9:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*)*)*(\\?([-_.!~*'()a-zA-Z0-9;/?:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*)?)?";
  text = "http://en.wikipedia.org/wiki/Parsing_expression_grammar";
  bench.push_back(testcase(regex, text, text, true));
  */
  
  uint64_t start, end;
  std::vector<benchresult> result(bench.size());
  for (std::size_t i = 0; i < bench.size(); i++) {
    regen::Regex r(bench[i].regex);
    start = rdtsc();
    r.Compile(olevel);
    end   = rdtsc();
    result[i].compile_time = end - start;
    start = rdtsc();
    result[i].result = r.FullMatch(bench[i].text) == bench[i].result;
    end   = rdtsc();
    result[i].matching_time = end - start;
  }

  const char *ostr[] = {"  Onone", "     O0", "     O1", "     O2", "     O3"};
  for (std::size_t i = 0; i < bench.size(); i++) {
    printf("BENCH %"PRIuS" : regex = /%s/ text = \"%s\"\n" , i, bench[i].regex.c_str(), bench[i].pretty.c_str());
    if (!result[i].result) puts("FAIL\n");
    printf("%s : compile time = %llu, matching time = %llu\n", ostr[olevel+1], result[i].compile_time, result[i].matching_time);
  }
  return 0;
}
