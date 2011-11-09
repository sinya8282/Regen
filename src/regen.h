#ifndef REGEN_H_
#define REGEN_H_

#include <string>

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
      ReverseMatch = 1 << 4,
      PartialMatch = 1 << 5,
      ParallelMatch = 1 << 6, // Enable Parallel Matching (SSFA)
      Extended = 1 << 7  // Regen-Extended syntax support (!,&,@)
    };
    enum CompileFlag {
      Onone = 0, O0, O1, O2, O3
    };
    Options(): flag_(NoParseFlags) {}
    Options(ParseFlag flag): flag_(flag) {}
    bool shortest() const { return flag_ & Shortest; }
    bool longest() const { return !shortest(); }
    bool dot_nl() const { return flag_ & DotNL; }
    bool match_nl() const { return flag_ & MatchNL; }
    bool one_line() const { return flag_ & MatchNL; }
    bool reverse_match() const { return flag_ & ReverseMatch; }
    bool partial_match() const { return flag_ & PartialMatch; }
    bool full_match() const { return !partial_match(); }
    bool parallel_match() const { return flag_ & ParallelMatch; }
    bool extended() const { return flag_ & Extended; }
 private:
    ParseFlag flag_;
  };
  Regen(const std::string &, const Regen::Options::ParseFlag);
  ~Regen();
  bool Match(const char *, const char *);
  static bool Match(const std::string & str, const Regex &reg, std::size_t) { return Match(str.c_str(), str.c_str()+str.length(), reg); }
  static bool Match(const char *, const char *, const Regex &);
  static bool FullMatch(const char *, const char *);
  static bool PartialMatch(const char *, const char *, const std::string &);
private:
  Regex *regex_;
  Regex *reverse_regex_;
};

inline Regen::Options::ParseFlag operator|(Regen::Options::ParseFlag a, Regen::Options::ParseFlag b)
{ return static_cast<Regen::Options::ParseFlag>(static_cast<int>(a) | static_cast<int>(b)); }
inline Regen::Options::ParseFlag operator&(Regen::Options::ParseFlag a, Regen::Options::ParseFlag b)
{ return static_cast<Regen::Options::ParseFlag>(static_cast<int>(a) & static_cast<int>(b)); }
inline Regen::Options::ParseFlag& operator|=(Regen::Options::ParseFlag &a, Regen::Options::ParseFlag b)
{ return a = static_cast<Regen::Options::ParseFlag>(static_cast<int>(a) | static_cast<int>(b)); }
inline Regen::Options::ParseFlag& operator&=(Regen::Options::ParseFlag &a, Regen::Options::ParseFlag b)
{ return a = static_cast<Regen::Options::ParseFlag>(static_cast<int>(a) & static_cast<int>(b)); }

} // namespace regen

using regen::Regen;

#endif // REGEN_H_
