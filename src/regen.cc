#include "regen.h"
#include "regex.h"

namespace regen {

Regen::Regen(const std::string &regex, const Regen::Options::ParseFlag options = Regen::Options::NoParseFlags)
{
  regex_ = new Regex(regex, options);
}

bool Regen::Match(const char *beg, const char *end)
{
  return regex_->Match(beg, end);
}

bool Regen::Match(const char *beg, const char *end, const Regex &reg)
{
  return reg.Match(beg, end);
}

bool Regen::FullMatch(const char *beg, const char *end)
{
  // Not yet
  return false;
}

bool Regen::PartialMatch(const char *beg, const char *end, const std::string &)
{
  // Not yet
  return false;
}

} // namespace regen
