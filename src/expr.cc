#include "expr.h"

namespace regen {

const char*
Expr::TypeString(Expr::Type type)
{
  static const char* const type_strings[] = {
    "Literal", "CharClass", "Dot", "BegLine",
    "EndLine", "EOP", "Concat", "Union", "Intersection",
    "Qmark", "Star", "Plus", "Repetition", "Rpar", "Lpar",
    "Epsilon", "None", "Complement"
  };

  return type_strings[type];
}
const char*
Expr::SuperTypeString(Expr::SuperType stype)
{
  static const char* const stype_strings[] = {
    "StateExpr", "BinaryExpr", "UnaryExpr"
  };

  return stype_strings[stype];
}

void Expr::Connect(std::set<StateExpr*> &src, std::set<StateExpr*> &dst) {
  std::set<StateExpr*>::iterator iter = src.begin();
  while (iter != src.end()) {
    (*iter)->transition().follow.insert(dst.begin(), dst.end());
    ++iter;
  }

  iter = dst.begin();
  while (iter != dst.end()) {
    (*iter)->transition().before.insert(src.begin(), src.end());
    ++iter;
  }
}

CharClass::CharClass(StateExpr *e1, StateExpr *e2):
    negative_(false)
{
  Expr *e = e1;
top:
  switch (e->type()) {
    case Expr::kLiteral:
      table_.set(((Literal*)e)->literal());
      break;
    case Expr::kCharClass:
      for (std::size_t i = 0; i < 256; i++) {
        if (((CharClass*)e)->Involve(i)) {
          table_.set(i);
        }
      }
      break;
    case Expr::kDot:
      table_.set();
      return;
    case Expr::kBegLine: case Expr::kEndLine:
      table_.set('\n');
      break;
    default: exitmsg("Invalid Expr Type: %d", e->type());
  }
  if (e == e1) {
    e = e2;
    goto top;
  }

  if (count() >= 128 && !negative()) {
    flip();
    set_negative(true);
  }
}

StateExpr::StateExpr():
    state_id_(0)
{
  min_length_ = max_length_ = 1;
  transition_.first.insert(this);
  transition_.last.insert(this);
}

Concat::Concat(Expr *lhs, Expr *rhs):
    BinaryExpr(lhs, rhs)
{
  if (lhs->max_length() == std::numeric_limits<size_t>::max() ||
      rhs->max_length() == std::numeric_limits<size_t>::max()) {
    max_length_ = std::numeric_limits<size_t>::max();
  } else {
    max_length_ = lhs->max_length() + rhs->max_length();
  }
  min_length_ = lhs->min_length() + rhs->min_length();

  transition_.first = lhs->transition().first;
  if (lhs->min_length() == 0) {
    transition_.first.insert(rhs->transition().first.begin(),
                             rhs->transition().first.end());
  }

  transition_.last = rhs->transition().last;
  if (rhs->min_length() == 0) {
    transition_.last.insert(lhs->transition().last.begin(),
                            lhs->transition().last.end());
  }
}

void Concat::FillTransition()
{
  Connect(lhs_->transition().last, rhs_->transition().first);
  rhs_->FillTransition();
  lhs_->FillTransition();
}

Union::Union(Expr *lhs, Expr *rhs):
    BinaryExpr(lhs, rhs)
{
  if (lhs->max_length() == std::numeric_limits<size_t>::max() ||
      rhs->max_length() == std::numeric_limits<size_t>::max()) {
    max_length_ = std::numeric_limits<size_t>::max();
  } else {
    max_length_ = std::max(lhs->max_length(), rhs->max_length());
  }
  min_length_ = std::min(lhs->min_length(), rhs->min_length());

  transition_.first = lhs->transition().first;
  transition_.first.insert(rhs->transition().first.begin(),
                           rhs->transition().first.end());

  transition_.last = lhs->transition().last;
  transition_.last.insert(rhs->transition().last.begin(),
                          rhs->transition().last.end());
}

void Union::FillTransition()
{
  rhs_->FillTransition();
  lhs_->FillTransition();
}

Qmark::Qmark(Expr* lhs):
    UnaryExpr(lhs)
{
  max_length_ = lhs->min_length();
  min_length_ = 0;
  transition_.first = lhs->transition().first;
  transition_.last = lhs->transition().last;
}

void Qmark::FillTransition()
{
  lhs_->FillTransition();
}

Plus::Plus(Expr* lhs):
    UnaryExpr(lhs)
{
  max_length_ = std::numeric_limits<size_t>::max();
  min_length_ = lhs->min_length();
  transition_.first = lhs->transition().first;
  transition_.last = lhs->transition().last;
}

void Plus::FillTransition()
{
  Connect(lhs_->transition().last, lhs_->transition().first);
  lhs_->FillTransition();
}

Star::Star(Expr* lhs):
    UnaryExpr(lhs)
{
  max_length_ = std::numeric_limits<size_t>::max();
  min_length_ = 0;
  transition_.first = lhs->transition().first;
  transition_.last = lhs->transition().last;
}

void Star::FillTransition()
{
  Connect(lhs_->transition().last, lhs_->transition().first);
  lhs_->FillTransition();
}

} // namespace regen
