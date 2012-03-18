#ifndef REGEN_H_
#define REGEN_H_

#include <string>
#include <string.h>

namespace regen {

class Regex;

class Regen {
public:
  class Options {
 public:
    enum ParseFlag {
      NoParseFlags = 0,
      IgnoreCase = 1 << 0,
      OneLine = 1 << 1,
      ReverseRegex = 1 << 2,
      ShortestMatch = 1 << 3,
      ReverseMatch = 1 << 4,
      Reverse = ReverseRegex | ReverseMatch,
      NoPrefixMatch = 1 << 5,
      NoSuffixMatch = 1 << 6,
      PartialMatch = NoPrefixMatch | NoSuffixMatch,
      FullMatch = 0,
      ParallelMatch = 1 << 7, // Enable Parallel Matching (SFA)
      CapturedMatch = 1 << 8,
      FilteredMatch = 1 << 9,
      /* Regen-Extended syntax support (!, &, @, &&, ||, #, \1) */
      ComplementExt = 1 << 10,
      IntersectionExt = 1 << 11,
      RecursionExt = 1 << 12,
      XORExt = 1 << 13,
      ShuffleExt = 1 << 14,
      PermutationExt = 1 << 15,
      ReverseExt = 1 << 16,
      WeakBackRefExt = 1 << 17,
      Extended =  ComplementExt | IntersectionExt | RecursionExt
      | XORExt | ShuffleExt | PermutationExt | ReverseExt | WeakBackRefExt,
      /* Encodings: UTF8 (ASCII is default) */
      EncodingUTF8 = 1 << 18,
      NonNullable = 1 << 19
    };
    enum CompileFlag {
      Onone = -1, O0 = 0, O1 = 1, O2 = 2, O3 = 3
    };
    Options(ParseFlag flag = NoParseFlags, const unsigned char delimiter = '\n');
    bool shortest_match() const { return shortest_match_; }
    void shortest_match(bool b) { shortest_match_ = b; }
    bool longest_match() const { return !shortest_match(); }
    void longest_match(bool b) { shortest_match_ = !b; }
    bool ignore_case() const { return ignore_case_; }
    void ignore_case(bool b) { ignore_case_ = b; }
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
    bool filtered_match() const { return filtered_match_ && !prefix_match(); }
    void filtered_match(bool b) { filtered_match_ = b; prefix_match(!b); }
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
    bool reverse_ext() const { return reverse_ext_; }
    void reverse_ext(bool b) { reverse_ext_ = b; }
    bool weakbackref_ext() const { return weakbackref_ext_; }
    void weakbackref_ext(bool b) { weakbackref_ext_ = b; }
    bool extended() const { return complement_ext_ & intersection_ext_ & recursion_ext_ & xor_ext_ & shuffle_ext_ & permutation_ext_ & reverse_ext_ & weakbackref_ext_; }
    void extended(bool b) { complement_ext_ = intersection_ext_ = recursion_ext_ = xor_ext_ = shuffle_ext_ = permutation_ext_ = reverse_ext_ = weakbackref_ext_ = b; }
    bool encoding_utf8() const { return encoding_utf8_; }
    void encoding_utf8(bool b) { encoding_utf8_ = b; }
    bool encoding_ascii() const { return !encoding_utf8(); }
    void encoding_ascii(bool b) { encoding_utf8(!b); }
    bool non_nullable() const { return non_nullable_; }
    void non_nullable(bool b) { non_nullable_ = b; }
    const unsigned char delimiter() const { return delimiter_; }
 private:
    bool shortest_match_;
    bool ignore_case_;
    bool one_line_;
    bool reverse_regex_;
    bool reverse_match_;
    bool noprefix_match_;
    bool nosuffix_match_;
    bool parallel_match_;
    bool captured_match_;
    bool filtered_match_;
    bool complement_ext_;
    bool intersection_ext_;
    bool recursion_ext_;
    bool xor_ext_;
    bool shuffle_ext_;
    bool permutation_ext_;
    bool reverse_ext_;
    bool weakbackref_ext_;
    bool encoding_utf8_;
    bool non_nullable_;
    const unsigned char delimiter_;
  };
  static const Options DefaultOptions;
  struct Context {
    Context() { clear(); }
    const char *ptr[2];
    const char * begin() const { return ptr[0]; }
    void set_begin(const char* p) { ptr[0] = p; }
    const char * end() const { return ptr[1]; }
    void set_end(const char* p) { ptr[1] = p; }
    const char * operator[](std::size_t index) const { return ptr[index]; }
    void clear() { ptr[0] = ptr[1] = NULL; }
  };
  struct StringPiece {
   public:
    StringPiece() { clear(); }
    StringPiece(const StringPiece &s) { set(s); }
    StringPiece(const char* str) { set(str); }
    StringPiece(const char* str, const char* end) { set(str, end); }
    StringPiece(const char* str, std::size_t length) { set(str, length); }
    StringPiece(const std::string& str) { set(str); }
    void set(const StringPiece &s) { set_begin(s.begin()); set_end(s.end()); }
    void set(const char* str, const char *end) { ptr[0] = str; ptr[1] = end; }
    void set(const char* str) { ptr[0] = str; ptr[1] = str == NULL ? str : str + strlen(str); }
    void set(const char* str, std::size_t length) { ptr[0] = str; ptr[1] = str + length; }
    void set(const std::string& str) { ptr[0] = str.data(); ptr[1] = str.data() + str.length(); }
    const char * data() const { return ptr[0]; }
    const unsigned char * udata() const { return (const unsigned char *)ptr[0]; }
    const char ** _data() const { return (const char**)ptr; }
    const unsigned char ** _udata() const { return (const unsigned char **)ptr; }
    const char * begin() const { return ptr[0]; }
    const unsigned char * ubegin() const { return (const unsigned char*)ptr[0]; }
    void set_begin(const char* p) { ptr[0] = p; }
    void set_ubegin(const unsigned char* p) { ptr[0] = (const char*)p; }
    const char * end() const { return ptr[1]; }
    const unsigned char * uend() const { return (const unsigned char*)ptr[1]; }
    void set_end(const char* p) { ptr[1] = p; }
    void set_uend(const unsigned char* p) { ptr[1] = (const char*)p; }
    const char * operator[](std::size_t index) const { return ptr[index]; }

    void consume(int dir = 1) { ptr[0] += dir; }
    void clear() { ptr[0] = ptr[1] = NULL; }
    void reverse() { if (!empty())
      { std::swap(ptr[0], ptr[1]); if (ptr[0] > ptr[1]) { ptr[0]--; ptr[1]--; } else { ptr[0]++; ptr[1]++; } }
    }
    bool valid() const { return ptr[0] != NULL && ptr[1] != NULL && ptr[0] <= ptr[1]; }
    std::size_t size() const { return !valid() ? 0 : ptr[1] - ptr[0]; }
    std::size_t length() const { return size(); }
    bool empty() const { return size() == 0; }
    std::string as_string() const { return std::string(ptr[0], size()); }
   private:
    const char *ptr[2];
  };
  Regen(const std::string &, Regen::Options = Regen::Options::NoParseFlags);
  ~Regen();
  bool Compile(Options::CompileFlag olevel = Options::O3);

  bool Match(const StringPiece& string, StringPiece* result = NULL) const;
  static bool Match(const StringPiece& string, const Regen& re, StringPiece* result = NULL) { return re.Match(string, result); }
  
  static bool FullMatch(const StringPiece& string, const StringPiece& pattern, Options opt, StringPiece *result = NULL);
  static bool FullMatch(const StringPiece& string, const StringPiece& pattern, StringPiece* result = NULL);

  static bool PartialMatch(const StringPiece &string, const StringPiece& pattern, Options opt, StringPiece *result = NULL);
  static bool PartialMatch(const StringPiece &string, const StringPiece& pattern, StringPiece *result = NULL);

  bool Consume(const StringPiece& string, StringPiece* result = NULL) const;
  static bool Consume(const StringPiece& string, const Regen& re, StringPiece* result = NULL) { return re.Consume(string, result); }
  static bool Consume(const StringPiece& string, const StringPiece& pattern, StringPiece* result = NULL);
  static bool Consume(const StringPiece& string, const StringPiece& pattern, Options opt, StringPiece* result = NULL);

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
