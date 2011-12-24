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
      ReverseRegex = 1 << 2,
      ShortestMatch = 1 << 3,
      ReverseMatch = 1 << 4,
      Reverse = ReverseRegex | ReverseMatch,
      NoPrefixMatch = 1 << 5,
      NoSuffixMatch = 1 << 6,
      PartialMatch = NoPrefixMatch | NoSuffixMatch,
      FullMatch = 0,
      ParallelMatch = 1 << 7, // Enable Parallel Matching (SSFA)
      CapturedMatch = 1 << 8,
      /* Regen-Extended syntax support (!, &, @, &&, ||, #, \1) */
      ComplementExt = 1 << 9,
      IntersectionExt = 1 << 10,
      RecursionExt = 1 << 11,
      XORExt = 1 << 12,
      ShuffleExt = 1 << 13,
      PermutationExt = 1 << 14,
      WeakBackRefExt = 1 << 15,
      Extended =  ComplementExt | IntersectionExt | RecursionExt
      | XORExt | ShuffleExt | PermutationExt | WeakBackRefExt,
      /* Encodings: UTF8 xor Ascii (default) */
      EncodingUTF8 = 1 << 14,
      EncodingAscii = 0
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
    bool reverse_regex() const { return reverse_regex_; }
    void reverse_regex(bool b) { reverse_regex_ = b; }
    bool reverse_match() const { return reverse_match_; }
    void reverse_match(bool b) { reverse_match_ = b; }
    bool reverse() const { return reverse_match() && reverse_regex(); }
    void reverse(bool b) { reverse_match(b); reverse_regex(b); }
    bool suffix_match() const { return !nosuffix_match_; }
    void suffix_match(bool b) { nosuffix_match_ = !b; }
    bool prefix_match() const { return !noprefix_match_; }
    void prefix_match(bool b) { noprefix_match_ = !b; }
    bool full_match() const { return prefix_match() && suffix_match(); }
    void full_match(bool b) { prefix_match(b); suffix_match(b); }
    bool partial_match() const { return !full_match(); }
    void partial_match(bool b) { full_match(!b); }
    bool parallel_match() const { return parallel_match_; }
    void parallel_match(bool b) { parallel_match_ = b; }
    bool captured_match() const { return captured_match_; }
    void captured_match(bool b) { captured_match_ = b; }
    bool complement_ext() const { return complement_ext_; }
    void complement_ext(bool b) { complement_ext_ = b; }
    bool intersection_ext() const { return intersection_ext_; }
    void intersection_ext(bool b) { intersection_ext_ = b; }
    bool recursion_ext() const { return recursion_ext_; }
    void recursion_ext(bool b) { recursion_ext_ = b; }
    bool xor_ext() const { return xor_ext_; }
    void xor_ext(bool b) { xor_ext_ = b; }
    bool shuffle_ext() const { return shuffle_ext_; }
    void shuffle_ext(bool b) { shuffle_ext_ = b; }
    bool permutation_ext() const { return permutation_ext_; }
    void permutation_ext(bool b) { permutation_ext_ = b; }
    bool weakbackref_ext() const { return weakbackref_ext_; }
    void weakbackref_ext(bool b) { weakbackref_ext_ = b; }
    bool extended() const { return complement_ext_ & intersection_ext_ & recursion_ext_ & xor_ext_ & shuffle_ext_ & permutation_ext_ & weakbackref_ext_; }
    void extended(bool b) { complement_ext_ = intersection_ext_ = recursion_ext_ = xor_ext_ = shuffle_ext_ = permutation_ext_ = weakbackref_ext_ = b; }
    bool encoding_utf8() const { return encoding_utf8_; }
    void encoding_utf8(bool b) { encoding_utf8_ = b; }
    bool encoding_ascii() const { return !encoding_utf8(); }
    void encoding_ascii(bool b) { encoding_utf8(!b); }
 private:
    bool shortest_match_;
    bool dot_nl_;
    bool match_nl_;
    bool one_line_;
    bool reverse_regex_;
    bool reverse_match_;
    bool noprefix_match_;
    bool nosuffix_match_;
    bool parallel_match_;
    bool captured_match_;
    bool complement_ext_;
    bool intersection_ext_;
    bool recursion_ext_;
    bool xor_ext_;
    bool shuffle_ext_;
    bool permutation_ext_;
    bool weakbackref_ext_;
    bool encoding_utf8_;
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
  Regen(const std::string &, Regen::Options = Regen::Options::NoParseFlags);
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
