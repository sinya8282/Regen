#ifndef REGEN_GENERATOR_H_
#define REGEN_GENERATOR_H_

#include "regex.h"
#include "dfa.h"

namespace regen {

namespace Generator {

void DotGenerate(const DFA &dfa);
void CGenerate(const DFA &dfa);

} // namespace Generator

} // namespace regen

#endif // REGEN_GENERATOR_H_
