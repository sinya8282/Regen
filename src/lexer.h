#ifndef REGEN_LEXER_H_
#define REGEN_LEXER_H_

#include "util.h"
#include "regen.h"

namespace regen {

class Lexer {
public:
  enum Type {
    kLiteral=0, kCharClass, kDot, kBegLine,
    kEndLine, kRecursive, kByteRange, kEOP,
    kConcat, kUnion, kIntersection,
    kQmark, kStar, kPlus, kRepetition,
    kRpar, kLpar, kEpsilon, kNone, kComplement
  };
Lexer(const unsigned char *begin, const unsigned char *end, Regen::Options flag = Regen::Options::NoParseFlags): ptr_(begin), begin_(begin), end_(end), flag_(flag) {}
  const unsigned char *ptr() { return ptr_; }
  Type Consume();
  bool Concatenated();
  bool Quantifier();
  Type token() { return token_; }
  const char * TokenToString(Type token);
  const char * TokenToString() { return TokenToString(token_); }
  unsigned char literal() { return literal_; }
  void literal(unsigned char l) { literal_ = l; }
  std::bitset<256> &table() { return table_; }
  std::pair<int, int> repetition() { return std::pair<int, int>(lower_repetition_, upper_repetition_); }
private:
  Type lex_repetition();
  Type lex_metachar();
  const unsigned char *ptr_;
  const unsigned char *begin_;
  const unsigned char *end_;
  int lower_repetition_, upper_repetition_;
  Type token_;
  std::bitset<256> table_;
  unsigned char literal_;
  Regen::Options flag_;
};

} // namespace regen

#endif // REGEN_LEXER_H_
