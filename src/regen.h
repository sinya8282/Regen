#ifndef REGEN_H_
#define REGEN_H_

#include "util.h"

namespace regen {

class Regex;

class Regen {
public:
  class Options {
    static const int DefaultOption;
 public:
    enum ParseFlag {
      NoParseFlags = 0,
      DotNL = 1 << 0,
      ClassNL = 1 << 1,
      MatchNL = ClassNL | DotNL,
      OneLine = 1 << 2,
      Shortest = 1 << 3,
      ParallelMatch = 1 << 4, // Enable Parallel Matching (SSFA)
      RegenExtended = 1 << 5  // Extended syntax support (!,&,@)
    };
    enum CompileFlag {
      Onone = 0, O0, O1, O2, O3
    };
  };
  Regen(const std::string &, const Regen::Options::ParseFlag);
  bool Match(const char *, const char *);
  static bool Match(const std::string & str, const Regex &reg, std::size_t) { return Match(str.c_str(), str.c_str()+str.length(), reg); }
  static bool Match(const char *, const char *, const Regex &);
  static bool FullMatch(const char *, const char *);
  static bool PartialMatch(const char *, const char *, const std::string &);
private:
  Regex *regex_;
};

inline Regen::Options::ParseFlag operator|(Regen::Options::ParseFlag a, Regen::Options::ParseFlag b)
{ return static_cast<Regen::Options::ParseFlag>(static_cast<int>(a) | static_cast<int>(b)); }

} // namespace regen

using regen::Regen;

#endif // REGEN_H_
