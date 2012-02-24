#include "regen.h"
#include "regex.h"

namespace regen {

const Regen::Options Regen::DefaultOptions(Regen::Options::NoParseFlags);

Regen::Options::Options(Regen::Options::ParseFlag flag, const unsigned char delimiter):
    shortest_match_(false), one_line_(false), reverse_regex_(false),
    reverse_match_(false), noprefix_match_(false), nosuffix_match_(false), parallel_match_(false),
    captured_match_(false), filtered_match_(false),
    complement_ext_(false), intersection_ext_(false), recursion_ext_(false), xor_ext_(false), shuffle_ext_(false),
    permutation_ext_(false), reverse_ext_(false), weakbackref_ext_(false),
    encoding_utf8_(false),
    delimiter_(delimiter)
{
  shortest_match_ = flag & ShortestMatch;
  ignore_case_ = flag & IgnoreCase;
  one_line_ = flag & OneLine;
  reverse_regex_ = flag & ReverseRegex;
  reverse_match_ = flag & ReverseMatch;
  noprefix_match_ = flag & NoPrefixMatch;
  nosuffix_match_ = flag & NoSuffixMatch;
  parallel_match_ = flag & ParallelMatch;
  captured_match_ = flag & CapturedMatch;
  filtered_match_ = flag & FilteredMatch;
  complement_ext_ = flag & ComplementExt;
  intersection_ext_ = flag & IntersectionExt;
  recursion_ext_ = flag & RecursionExt;
  xor_ext_ = flag & XORExt;
  shuffle_ext_ = flag & ShuffleExt;
  permutation_ext_ = flag & PermutationExt;
  reverse_ext_ = flag & ReverseExt;
  weakbackref_ext_ = flag & WeakBackRefExt;
  encoding_utf8_ = flag & EncodingUTF8;
}

Regen::Regen(const std::string &regex, const Regen::Options options):
    regex_(NULL), reverse_regex_(NULL), flag_(options)
{
  regex_ = new Regex(regex, flag_);
  if (flag_.captured_match() && !flag_.prefix_match()
      && regex_->min_length() != regex_->max_length()) {
    Options opt(flag_);
    opt.reverse(true);
    opt.prefix_match(true);
    opt.suffix_match(false);
    opt.longest_match(true);
    opt.captured_match(false);
    reverse_regex_ = new Regex(regex, opt);
  }
}

Regen::~Regen()
{
  delete regex_;
  delete reverse_regex_;
}

bool Regen::Compile(Options::CompileFlag olevel)
{
  bool compile = regex_->Compile(olevel);
  if (reverse_regex_ != NULL) {
    compile &= reverse_regex_->Compile(olevel);
  }
  return compile;
}

bool Regen::Match(const StringPiece &string, StringPiece *result) const
{
  if (result != NULL && flag_.captured_match()) {
    bool match = regex_->Match(string, result);
    if (result->end() != NULL) {
      if (flag_.suffix_match()) {
        result->set_begin(string.begin());
      } else if (flag_.prefix_match()) {
        result->set_begin(string.begin());
      } else if (reverse_regex_ == NULL) {
        result->set_begin(result->end() - regex_->min_length());
      } else {
        StringPiece string_(string.begin(), result->end());
        reverse_regex_->Match(string_, result);
      }
    }
    return match;
  } else {
    return regex_->Match(string, result);    
  }
}

bool Regen::FullMatch(const StringPiece& string, const StringPiece& pattern, StringPiece *result)
{
  return FullMatch(string, pattern, DefaultOptions, result);
}

bool Regen::FullMatch(const StringPiece& string, const StringPiece &pattern, Options opt, StringPiece *result)
{
  opt.full_match(true);
  Regex re(pattern, opt);
  re.Compile();
  return re.Match(string, result);
}

bool Regen::PartialMatch(const StringPiece& string, const StringPiece &pattern, StringPiece *result)
{
  return PartialMatch(string, pattern, DefaultOptions, result);
}

bool Regen::PartialMatch(const StringPiece& string, const StringPiece& pattern, Options opt, StringPiece *result)
{
  opt.partial_match(true);
  Regex re(pattern, opt);
  re.Compile();
  return re.Match(string, result);
}

} // namespace regen
