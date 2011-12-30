#include "regen.h"
#include "regex.h"

namespace regen {

const Regen::Options Regen::DefaultOptions(Regen::Options::NoParseFlags);

Regen::Options::Options(Regen::Options::ParseFlag flag):
    shortest_match_(false), match_nl_(false), one_line_(false), reverse_regex_(false),
    reverse_match_(false), noprefix_match_(false), nosuffix_match_(false), parallel_match_(false),
    captured_match_(false), complement_ext_(false), intersection_ext_(false),
    recursion_ext_(false), xor_ext_(false), shuffle_ext_(false),
    permutation_ext_(false), reverse_ext_(false), weakbackref_ext_(false),
    encoding_utf8_(false)
{
  shortest_match_ = flag & ShortestMatch;
  match_nl_ = flag & MatchNL;
  one_line_ = flag & OneLine;
  reverse_regex_ = flag & ReverseRegex;
  reverse_match_ = flag & ReverseMatch;
  noprefix_match_ = flag & NoPrefixMatch;
  nosuffix_match_ = flag & NoSuffixMatch;
  parallel_match_ = flag & ParallelMatch;
  captured_match_ = flag & CapturedMatch;
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
  if (flag_.captured_match() && !flag_.prefix_match()) {
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

bool Regen::Match(const char *beg, const char *end, Context *context) const
{
  if (context != NULL && flag_.captured_match()) {
    bool match = regex_->Match(beg, end, context);
    if (context->end() != NULL) {
      if (flag_.full_match()) {
        context->set_begin(beg);
      } else if (flag_.prefix_match()) {
        context->set_begin(beg);
      } else {
        Regen::Context context_;
        reverse_regex_->Match(context->end()-1, beg-1, &context_);
        context->set_begin(context_.end()+1);
      }
    }
    return match;
  } else {
    return regex_->Match(beg, end, context);    
  }
}

bool Regen::Match(const char *beg, const char *end, const Regen &reg, Context *context)
{
  return reg.Match(beg, end, context);
}

bool Regen::FullMatch(const char *beg, const char *end, const std::string &reg, Context *context)
{
  return FullMatch(beg, end, reg, DefaultOptions, context);
}

bool Regen::FullMatch(const char *beg, const char *end, const std::string &reg, Options opt, Context *context)
{
  opt.full_match(true);
  Regex re(reg, opt);
  re.Compile();
  return re.Match(beg, end, context);
}

bool Regen::PartialMatch(const char *beg, const char *end, const std::string &reg, Context *context)
{
  return PartialMatch(beg, end, reg, DefaultOptions, context);
}

bool Regen::PartialMatch(const char *beg, const char *end, const std::string &reg, Options opt, Context *context)
{
  opt.partial_match(true);
  Regex re(reg, opt);
  re.Compile();
  return re.Match(beg, end, context);
}

} // namespace regen
