#ifndef REGEN_GENERATOR_H_
#define REGEN_GENERATOR_H_

#include "regex.h"

namespace regen {

namespace Generator {

void DotGenerate(const Regex &regex);
void CGenerate(const Regex &regex);

} // namespace Generator

} // namespace regen

#endif // REGEN_GENERATOR_H_
