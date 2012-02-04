#include "../regen.h"
#include "../regex.h"

struct testcase {
  testcase(std::string regex_, std::string text_, bool result_): regex(regex_), text(text_), result(result_) {}
  std::string regex;
  std::string text;
  bool result;
};

int main(int argc, char *argv[]) {
  int opt;
  std::size_t thread_num = 1;
  Regen::Options::CompileFlag olevel = Regen::Options::O3;

  while ((opt = getopt(argc, argv, "nf:t:O:t:")) != -1) {
    switch(opt) {
      case 'O': {
        olevel = Regen::Options::CompileFlag(atoi(optarg));
        break;
      }
      case 't': {
        thread_num = atoi(optarg);
        break;
      }
    }
  }

  testcase test[] = {
    testcase("abc", "abc", true),
    testcase("abc", "xbc", false),
    testcase("abc", "axc", false),
    testcase("abc", "abx", false),
    testcase(".*abc.*", "xabcy", true),
    testcase(".*abc", "ababc", true),
    testcase("ab*c", "abc", true),
    testcase("ab*bc", "abc", true),
    testcase("ab*bc", "abbc", true),
    testcase("ab*bc", "abbbbc", true),
    testcase("ab+bc", "abbc", true),
    testcase("ab+bc", "abc", false),
    testcase("ab+bc", "abq", false),
    testcase("ab+bc", "abbbbc", true),
    testcase("ab?bc", "abbc", true),
    testcase("ab?bc", "abc", true),
    testcase("ab?bc", "abbbbc", false),
    testcase("ab?c", "abc", true),
    //testcase("^abc$", "abc", true),
    //testcase("^abc$", "abcc", false),
    //testcase("^abc", "abcc", true),
    //testcase("^abc$", "aabc", false),
    //testcase("abc$", "aabc", true),
    //testcase("$", "abc", true),
    testcase("a.c", "abc", true),
    testcase("a.c", "axc", true),
    testcase("a.*c", "axyzc", true),
    testcase("a.*c", "axyzd", false),
    testcase("a[bc]d", "abc", false),
    testcase("a[bc]d", "abd", true),
    testcase("a[b-d]e", "abd", false),
    testcase("a[b-d]e", "ace", true),
    testcase(".*a[b-d]", "aac", true),
    testcase("a[-b]", "a-", true),
    testcase("a[b-]", "a-", true),
    testcase("a]", "a]", true),
    testcase("a[]]b", "a]b", true),
    testcase("a[^bc]d", "aed", true),
    testcase("a[^bc]d", "abd", false),
    testcase("a[^-b]c", "adc", true),
    testcase("a[^-b]c", "a-c", false),
    testcase("a[^]b]c", "a]c", false),
    testcase("a[^]b]c", "adc", true),
    testcase("ab|cd", "abc", false),
    testcase("ab|cd", "abcd", false),
    testcase("$b", "b", false),
    testcase("a\\(b", "a(b", true),
    testcase("a\\(*b", "ab", true),
    testcase("a\\(*b", "a((b", true),
    testcase("a\\x", "a\\x", false),
    testcase("((a))", "a", true),
    testcase("(a)b(c)", "abc", true),
    testcase("a+b+c", "aabbbc", true),
    testcase("a**", "", true),
    testcase("a*?", "", true),
    testcase("(a*)*", "", true),
    testcase("(a*)+", "", true),
    testcase("(a*|b)*", "", true),
    testcase("(a+|b)*", "ab", true),
    testcase("(a+|b)+", "ab", true),
    testcase(".*(a+|b)?", "ab", true),
    testcase("[^ab]*", "cde", true),
    testcase("abc", "", false),
    testcase("abc", "", false),
    testcase("a*", "", true),
    testcase("([abc])*d", "abbbcd", true),
    testcase("([abc])*bcd", "abcd", true),
    testcase("a|b|c|d|e", "e", true),
    testcase("(a|b|c|d|e)f", "ef", true),
    testcase("((a*|b))*", "", true),
    testcase("abcd*efg", "abcdefg", true),
    testcase("ab*", "xabyabbbz", false),
    testcase("ab*", "xayabbbz", false),
    testcase(".*(ab|cd)e", "abcde", true),
    testcase("[abhgefdc]ij", "hij", true),
    testcase("^(ab|cd)e", "abcde", false),
    testcase(".*(a|b)c*d", "abcd", true),
    testcase("(ab|ab*)bc", "abc", true),
    testcase("a([bc]*)c*", "abc", true),
    testcase("a([bc]*)(c*d)", "abcd", true),
    testcase("a([bc]+)(c*d)", "abcd", true),
    testcase("a([bc]*)(c+d)", "abcd", true),
    testcase("a[bcd]*dcdcde", "adcdcde", true),
    testcase("a[bcd]+dcdcde", "adcdcde", false),
    testcase("(ab|a)b*c", "abc", true),
    testcase("((a)(b)c)(d)", "abcd", true),
    testcase("[A-Za-z_][A-Za-z1-9_]*", "alpha", true),
    //testcase("^a(bc+|b[eh])g|.h$", "abh", true),
    testcase("(bc+d$|ef*g.|h?i(j|k))", "effgz", true),
    testcase("(bc+d$|ef*g.|h?i(j|k))", "ij", true),
    testcase("(bc+d$|ef*g.|h?i(j|k))", "effg", false),
    testcase("(bc+d$|ef*g.|h?i(j|k))", "bcdd", false),
    testcase(".*(bc+d$|ef*g.|h?i(j|k))", "reffgz", true),
    testcase("((((((((((a))))))))))", "-", false),
    testcase("(((((((((a)))))))))", "a", true),
    testcase("multiple words of text", "uh-uh", false),
    testcase("multiple words.*", "multiple words, yeah", true),
    testcase("(.*)c(.*)", "abcde", true),
    testcase("\\((.*),", "(.*)\\)", false),
    testcase("[k]", "ab", false),
    testcase("abcd", "abcd", true),
    testcase("a(bc)d", "abcd", true),
    testcase("a[-]?c", "ac", true)
  };

  const std::size_t TESTNUM = sizeof(test) / sizeof(testcase);
  bool result[TESTNUM];
  std::size_t fail = 0;
  for (std::size_t i = 0; i < TESTNUM; i++) {
    regen::Regex r(test[i].regex);
    if (thread_num <= 1) {
      r.Compile(olevel);
      result[i] = r.Match(test[i].text) == test[i].result;
    } else {
#ifdef REGEN_ENABLE_PARALLEL
      regen::SFA pdfa(r.expr_root(), r.state_exprs(), thread_num);
      pdfa.Compile(olevel);
      result[i] = pdfa.Match(test[i].text) == test[i].result;
#else
      exitmsg("SFA is not supported.");
#endif
    }
  }

  for (std::size_t i = 0; i < TESTNUM; i++) {
    if (!result[i]) {
      printf("TEST %"PRIuS" FAILED: regex = /%s/, text = \"%s\", expect = %s\n"
             , i, test[i].regex.c_str(), test[i].text.c_str(), (test[i].result ? "true" : "false"));
      fail++;
    }
  }
  if (fail == 0) {
    puts("ALL TEST PASSED");
  } else {
    printf("%"PRIuS" TESTS FAILED\n", fail);
  }
  
  return 0;
}
