#include "regen.h"
#include "regex.h"

namespace regen {

Regen::Regen(const std::string &regex, const int option)
    :regex_(NULL)
{
  regex_ = new Regex(regex);
}

bool Regen::Match(const char *beg, const char *end)
{
  return regex_->Match(beg, end);
}

bool Regen::Match(const char *beg, const char *end, const Regex &reg)
{
  return reg.Match(beg, end);
}

bool Regen::FullMatch(const char *beg, const char *end, const std::string &reg)
{
  // Not yet
  return false;
}

bool Regen::PartialMatch(const char *beg, const char *end, const std::string &reg)
{
  // Not yet
  return false;
}

} // namespace regen
