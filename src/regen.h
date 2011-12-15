#ifndef REGEN_H_
#define REGEN_H_

#include <string>

namespace regen {

class Regex;

class Regen {
public:
  class Options {
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
      /* Regen-Extended syntax support (!, &, @, &&, ||, #, \1) */
      ComplementExt = 1 << 7,
      IntersectionExt = 1 << 8,
      RecursionExt = 1 << 9,
      XORExt = 1 << 10,
      ShuffleExt = 1 << 11,
      PermutationExt = 1 << 12,
      WeakBackRefExt = 1 << 13,
      Extended =  ComplementExt | IntersectionExt | RecursionExt
      | XORExt | ShuffleExt | PermutationExt | WeakBackRefExt
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
    bool complement_ext() { return complement_ext_; }
    void complement_ext(bool b) { complement_ext_ = b; }
    bool intersection_ext() { return intersection_ext_; }
    void intersection_ext(bool b) { intersection_ext_ = b; }
    bool recursion_ext() { return recursion_ext_; }
    void recursion_ext(bool b) { recursion_ext_ = b; }
    bool xor_ext() { return xor_ext_; }
    void xor_ext(bool b) { xor_ext_ = b; }
    bool shuffle_ext() { return shuffle_ext_; }
    void shuffle_ext(bool b) { shuffle_ext_ = b; }
    bool permutation_ext() { return permutation_ext_; }
    void permutation_ext(bool b) { permutation_ext_ = b; }
    bool weakbackref_ext() { return weakbackref_ext_; }
    void weakbackref_ext(bool b) { weakbackref_ext_ = b; }
 private:
    bool shortest_match_;
    bool dot_nl_;
    bool match_nl_;
    bool one_line_;
    bool reverse_match_;
    bool partial_match_;
    bool parallel_match_;
    bool captured_match_;
    bool complement_ext_;
    bool intersection_ext_;
    bool recursion_ext_;
    bool xor_ext_;
    bool shuffle_ext_;
    bool permutation_ext_;
    bool weakbackref_ext_;
  };
  static const Options DefaultOptions;
  struct Context {
    Context() { ptr[0] = ptr[1] = NULL; }
    const char *ptr[2];
    const char * begin() const { return ptr[0]; }
    void set_begin(const char* p) { ptr[0] = p; }
    const char * end() const { return ptr[1]; }
    void set_end(const char* p) { ptr[1] = p; }
    const char * operator[](std::size_t index) const { return ptr[index]; }
  };
  Regen(const std::string &, Regen::Options);
  ~Regen();
  bool Compile(Options::CompileFlag olevel = Options::O3);

  bool Match(const char *, const char *, Context *context = NULL) const;
  static bool Match(const char *, const char *, const Regen &, Context * context = NULL);
  static bool FullMatch(const char *, const char *, const std::string &, Options, Context *context = NULL);
  static bool PartialMatch(const char *, const char *, const std::string &, Options, Context *context = NULL);
  static bool FullMatch(const char *, const char *, const std::string &, Context *context = NULL);
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
