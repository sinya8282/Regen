#include "regen.h"
#include "regex.h"

namespace regen {

Regen::Options::Options(Regen::Options::ParseFlag flag):
    shortest_match_(false), match_nl_(false), one_line_(false),
    reverse_match_(false), partial_match_(false), parallel_match_(false),
    captured_match_(false), extended_(false)
{
  if (flag & ShortestMatch) shortest_match_ = true;
  if (flag & MatchNL) match_nl_ = true;
  if (flag & OneLine) one_line_ = true;
  if (flag & ReverseMatch) reverse_match_ = true;
  if (flag & PartialMatch) partial_match_ = true;
  if (flag & ParallelMatch) parallel_match_ = true;
  if (flag & CapturedMatch) captured_match_ = true;
  if (flag & Extended) extended_ = true;
}

Regen::Regen(const std::string &regex, const Regen::Options options = Regen::Options::NoParseFlags):
    regex_(NULL), reverse_regex_(NULL), flag_(options)
{
  regex_ = new Regex(regex, flag_);
  if (flag_.captured_match() && flag_.partial_match()) {
    Options opt(flag_);
    opt.reverse_match(true);
    opt.partial_match(false);
    opt.shortest_match(false);
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
    if (context->end != NULL) {
      if (flag_.full_match()) {
        context->begin = beg;
      } else {
        Regen::Context context_;
        reverse_regex_->Match(context->end-1, beg-1, &context_);
        context->begin = context_.end+1;
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

bool Regen::FullMatch(const char *beg, const char *end, Context *context)
{
  // Not yet
  return false;
}

bool Regen::PartialMatch(const char *beg, const char *end, const std::string &, Context *context)
{
  // Not yet
  return false;
}

} // namespace regen
