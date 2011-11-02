#ifndef REGEN_H_
#define REGEN_H_

#include "util.h"

namespace regen {

class Regex;

class Regen {
public:
  class Options {
 public:
    enum ParseFlag {
      NoParseFlags = 0,
      DotNL = 1 << 0,
      ClassNL = 1 << 1,
      MatchNL = ClassNL | DotNL,
      OneLine = 1 << 2,
      Shortest = 1 << 3,
      RegenX = 1 << 4 // Regen Extensions (!,&,@)
    };
    enum CompileFlag {
      Onone = 0, O0, O1, O2, O3
    };
  };
  Regen(const std::string &, const int);
  bool Match(const char *, const char *);
  static bool Match(const std::string & str, const Regex &reg, std::size_t) { return Match(str.c_str(), str.c_str()+str.length(), reg); }
  static bool Match(const char *, const char *, const Regex &);
  static bool FullMatch(const char *, const char *);
  static bool PartialMatch(const char *, const char *, const std::string &);
private:
  Regex *regex_;
};

} // namespace regen

using regen::Regen;

#endif // REGEN_H_
