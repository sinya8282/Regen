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
      MatchNL = 1 << 0,
      OneLine = 1 << 1,
      ShortestMatch = 1 << 2,
      ReverseMatch = 1 << 3,
      PartialMatch = 1 << 4,
      ParallelMatch = 1 << 5, // Enable Parallel Matching (SSFA)
      CapturedMatch = 1 << 6,
      Extended = 1 << 7  // Regen-Extended syntax support (!,&,@)
    };
    enum CompileFlag {
      Onone = -1, O0 = 0, O1 = 1, O2 = 2, O3 = 3
    };
    Options(ParseFlag flag = NoParseFlags);
    bool shortest_match() const { return shortest_match_; }
    void shortest_match(bool b) { shortest_match_ = b; }
    bool longest_match() const { return !shortest_match(); }
    void longest_match(bool b) { shortest_match_ = !b; }
    bool match_nl() const { return match_nl_; }
    void match_nl(bool b) { match_nl_ = b; }
    bool one_line() const { return one_line_; }
    void one_line(bool b) { one_line_ = b; }
    bool reverse_match() const { return reverse_match_; }
    void reverse_match(bool b) { reverse_match_ = b; }
    bool partial_match() const { return partial_match_; }
    void partial_match(bool b) { partial_match_ = b; }
    bool full_match() const { return !partial_match(); }
    void full_match(bool b) { partial_match_ = !b; }
    bool parallel_match() const { return parallel_match_; }
    void parallel_match(bool b) { parallel_match_ = b; }
    bool captured_match() const { return captured_match_; }
    void captured_match(bool b) { captured_match_ = b; }    
    bool extended() { return extended_; }
    void extended(bool b) { extended_ = b; }        
 private:
    bool shortest_match_;
    bool dot_nl_;
    bool match_nl_;
    bool one_line_;
    bool reverse_match_;
    bool partial_match_;
    bool parallel_match_;
    bool captured_match_;
    bool extended_;
  };
  struct Context {
    Context(): begin(NULL), end(NULL), state(-1) {}
    const char *begin;
    const char *end;
    uint64_t state;
  };
  Regen(const std::string &, Regen::Options);
  ~Regen();
  bool Compile(Options::CompileFlag olevel = Options::O3);
  bool Match(const char *, const char *, Context *context = NULL) const;
  static bool Match(const char *, const char *, const Regen &, Context * context = NULL);
  static bool FullMatch(const char *, const char *, Context *context = NULL);
  static bool PartialMatch(const char *, const char *, const std::string &, Context *context = NULL);
private:
  Regex *regex_;
  Regex *reverse_regex_;
  Options flag_;
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
