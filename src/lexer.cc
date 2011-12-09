#include "lexer.h"

namespace regen {

Lexer::Type Lexer::Consume()
{
  if (ptr_ >= end_) {
    token_ = kEOP;
    return token_;
  }

  switch (literal_ = *ptr_++) {
    // Regen Extension
    case '@': token_ = kRecursive; break;
    case '!': token_ = kComplement;  break;
    case '&': {
      if (*ptr_ == '&') {
        ptr_++;
        token_ = kXOR;
      } else {
        token_ = kIntersection;
      }
      break;
    }
    // Normal Symbols
    case '.': token_ = kDot;       break;
    case '[': token_ = kCharClass; break;
    case '|': token_ = kUnion;     break;
    case '?': token_ = kQmark;     break;
    case '+': token_ = kPlus;      break;
    case '*': token_ = kStar;      break;
    case ')': token_ = kRpar;      break;
    case '^': token_ = kBegLine;   break;
    case '$': token_ = kEndLine;   break;
    case '(': {
      if (*ptr_ == ')') {
        ptr_++;
        token_ = kNone;
      } else {
        token_ = kLpar;
      }
      break;
    }
    case '{':
      token_ = lex_repetition();
      break;
    case '\\':
      token_ = lex_metachar();
      break;
    default:
      token_ = kLiteral;
  }

  return token_;
}

Lexer::Type Lexer::lex_metachar()
{
  if (*ptr_ == '_' && '0' <= *(ptr_+1) && *(ptr_+1) <= '9') ptr_++;
  if ('0' <= *ptr_ && *ptr_ <= '9') {
    weakref_ = (*(ptr_-1) == '_');
    backref_ = 0;
    do {
      backref_ *= 10;
      backref_ += *ptr_++ - '0';
    } while ('0' <= *ptr_ && *ptr_ <= '9');
    backref_--;
    return kBackRef;
  }

  Type token;
  literal_ = *ptr_++;
  switch (literal_) {
    case '\0': exitmsg("bad '\\'");
    case 'a': /* bell */
      literal_ = '\a';
      token = kLiteral;
      break;      
    case 'd': /*digits /[0-9]/  */
    case 'D': // or not
      table_.reset();
      for (unsigned char c = '0'; c <= '9'; c++) {
        table_.set(c);
      }
      if (literal_ == 'D') table_.flip();
      token = kByteRange;
      break;
    case 'f': /* form feed */
      literal_ = '\f';
      token = kLiteral;
      break;
    case 'n': /* new line */
      literal_ = '\n';
      token = kLiteral;
      break;
    case 'r': /* carriage retur */
      literal_ = '\r';
      token = kLiteral;
      break;
    case 's': /* whitespace [\t\n\f\r ] */
    case 'S': // or not
      table_.reset();
      table_.set('\t'); table_.set('\n'); table_.set('\f');
      table_.set('\r'); table_.set(' ');
      if (literal_ == 'S') table_.flip();
      token = kByteRange;
      break;
    case 't': /* horizontal tab */
      literal_ = '\t';
      token = kLiteral;
      break;
    case 'v': /* vertical tab */
      literal_ = '\v';
      token = kLiteral;
      break;
    case 'w': /* word characters [0-9A-Za-z_] */
    case 'W': // or not
      table_.reset();
      for (unsigned char c = '0'; c <= '9'; c++) {
        table_.set(c);
      }
      for (unsigned char c = 'A'; c <= 'Z'; c++) {
        table_.set(c);
      }
      for (unsigned char c = 'a'; c <= 'z'; c++) {
        table_.set(c);
      }
      table_.set('_');
      if (literal_ == 'W') table_.flip();
      token = kByteRange;
      break;
    case 'x': {
      unsigned char hex = 0;
      for (int i = 0; i < 2; i++) {
        literal_ = *ptr_++;
        hex <<= 4;
        if ('A' <= literal_ && literal_ <= 'F') {
          hex += literal_ - 'A' + 10;
        } else if ('a' <= literal_ && literal_ <= 'f') {
          hex += literal_ - 'a' + 10;
        } else if ('0' <= literal_ && literal_ <= '9') {
          hex += literal_ - '0';
        } else {
          if (i == 0) {
            hex = 0;
          } else {
            hex >>= 4;
          }
          ptr_--;
          break;
        }
      }
      token = kLiteral;
      literal_ = hex;
      break;
    }
    default:
      token = kLiteral;
      break;
  }

  return token;
}

Lexer::Type Lexer::lex_repetition()
{
  const unsigned char *ptr = ptr_;
  lower_repetition_ = 0;

  if ('0' <= *ptr && *ptr <= '9') {
    do {
      lower_repetition_ *= 10;
      lower_repetition_ += *ptr++ - '0';
    } while ('0' <= *ptr && *ptr <= '9');
  } else if (*ptr == ',') {
    lower_repetition_ = 0;
  } else {
    goto illformed;
  }
  if (*ptr == ',') {
    upper_repetition_ = 0;
    ptr++;
    if ('0' <= *ptr && *ptr <= '9') {
      do {
        upper_repetition_ *= 10;
        upper_repetition_ += *ptr++ - '0';
      } while ('0' <= *ptr && *ptr <= '9');
      if (*ptr != '}') {
        goto illformed;
      }
    } else if (*ptr == '}') {
      upper_repetition_ = -1;
    } else {
      goto illformed;
    }
  } else if (*ptr == '}') {
    upper_repetition_ = lower_repetition_;
  } else {
    goto illformed;
  }
  ptr_ = ++ptr;
  if (lower_repetition_ == 0 && upper_repetition_ == -1) {
    token_ = kStar;
  } else if (lower_repetition_ == 1 && upper_repetition_ == -1) {
    token_ = kPlus;
  } else if (lower_repetition_ == 0 && upper_repetition_ == 1) {
    token_ = kQmark;
  } else if (lower_repetition_ == 1 && upper_repetition_ == 1) {
    token_ = Consume();
  } else if (upper_repetition_ != -1 && upper_repetition_ < lower_repetition_) {
    exitmsg("Invalid repetition quantifier {%d,%d}",
            lower_repetition_, upper_repetition_);
  } else {
    token_ = kRepetition;
  }
  return token_;

illformed:
  token_ = kLiteral;
  return token_;
}

bool Lexer::Concatenated()
{
  switch (token_) {
    case kLiteral: case kCharClass: case kDot:
    case kEndLine: case kBegLine: case kNone:
    case kLpar: case kComplement: case kRecursive:
    case kByteRange: case kBackRef:
      return true;
    default:
      return false;
  }
}

bool Lexer::Quantifier()
{
  switch (token_) {
    case kStar: case kPlus: case kQmark:
    case kRepetition:
      return true;
    default:
      return false;
  }
}

const char* Lexer::TokenToString(Lexer::Type token)
{
  static const char* str[] = {
    "kLiteral", "kCharClass", "kDot", "kBegLine", "kEndLine",
    "kRecursive", "kByteRange", "kEOP", "kConcat", "kUnion",
    "kIntersection", "kQmark", "kStar", "kPlus", "kRepetition",
    "kRpar", "kLpar", "kEpsilon", "kNone", "kComplement"
  };

  return str[token];
}

} // namespace regen
