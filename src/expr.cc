#include "expr.h"

namespace regen {

const char*
Expr::TypeString(Expr::Type type)
{
  static const char* const type_strings[] = {
    "Literal", "CharClass", "Dot", "BegLine", "EndLine",
    "EOP", "Concat", "Union", "Qmark", "Star", "Plus",
    "Epsilon", "None"
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

void Expr::Connect(std::set<StateExpr*> &src, std::set<StateExpr*> &dst, bool reverse)
{
  if (reverse) {
    std::set<StateExpr*>::iterator iter = dst.begin();
    while (iter != dst.end()) {
      (*iter)->transition().follow.insert(src.begin(), src.end());
      ++iter;
    }
  } else {
    std::set<StateExpr*>::iterator iter = src.begin();
    while (iter != src.end()) {
      (*iter)->transition().follow.insert(dst.begin(), dst.end());
      ++iter;
    }
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

void Concat::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);
  rhs_->FillPosition(info);

  max_length_ = lhs_->max_length() + rhs_->max_length();
  min_length_ = lhs_->min_length() + rhs_->min_length();
  nullable_ = lhs_->nullable() && rhs_->nullable();

  transition_.first = lhs_->transition().first;

  if (lhs_->nullable()) {
    transition_.first.insert(rhs_->transition().first.begin(),
                             rhs_->transition().first.end());
  }

  transition_.last = rhs_->transition().last;

  if (rhs_->nullable()) {
    transition_.last.insert(lhs_->transition().last.begin(),
                            lhs_->transition().last.end());
  }
}

void Concat::FillTransition(bool reverse)
{
  Connect(lhs_->transition().last, rhs_->transition().first, reverse);
  rhs_->FillTransition(reverse);
  lhs_->FillTransition(reverse);
}

void Union::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);
  rhs_->FillPosition(info);
  
  max_length_ = std::max(lhs_->max_length(), rhs_->max_length());
  min_length_ = std::min(lhs_->min_length(), rhs_->min_length());
  nullable_ = lhs_->nullable() || rhs_->nullable();

  transition_.first = lhs_->transition().first;
  transition_.first.insert(rhs_->transition().first.begin(),
                           rhs_->transition().first.end());

  transition_.last = lhs_->transition().last;
  transition_.last.insert(rhs_->transition().last.begin(),
                          rhs_->transition().last.end());
}

void Union::FillTransition(bool reverse)
{
  rhs_->FillTransition(reverse);
  lhs_->FillTransition(reverse);
}

Intersection::Intersection(Expr *lhs, Expr *rhs):
    BinaryExpr(lhs, rhs), lhs__(lhs), rhs__(rhs)
{
  Operator::NewPair(&lop_, &rop_, Operator::kIntersection);
  lhs_ = new Concat(lhs_, lop_);
  rhs_ = new Concat(rhs_, rop_);  
}

void Intersection::FillPosition(ExprInfo* info)
{
  lhs_->FillPosition(info);
  rhs_->FillPosition(info);

  nullable_ = lhs__->nullable() && rhs__->nullable();
  max_length_ = std::min(lhs_->max_length(), rhs_->max_length());
  min_length_ = std::max(lhs_->min_length(), rhs_->min_length());

  transition_.first = lhs_->transition().first;
  transition_.first.insert(rhs_->transition().first.begin(),
                           rhs_->transition().first.end());

  transition_.last = lhs_->transition().last;
  transition_.last.insert(rhs_->transition().last.begin(),
                          rhs_->transition().last.end());
}

void Intersection::FillTransition(bool reverse)
{
  rhs_->FillTransition(reverse);
  lhs_->FillTransition(reverse);
}

XOR::XOR(Expr* lhs, Expr* rhs):
    BinaryExpr(lhs, rhs), lhs__(lhs), rhs__(rhs)
{
  Operator::NewPair(&lop_, &rop_, Operator::kXOR);
  lhs_ = new Concat(lhs_, lop_);
  rhs_ = new Concat(rhs_, rop_);
}

void XOR::FillPosition(ExprInfo *info)
{  
  lhs_->FillPosition(info);
  rhs_->FillPosition(info);

  nullable_ = lhs__->nullable() ^ rhs__->nullable();
  max_length_ = std::numeric_limits<size_t>::max();
  if (lhs_->min_length() == 0 && rhs_->min_length() == 0) {
    min_length_ = std::numeric_limits<size_t>::max();
  } else {
    min_length_ = std::min(lhs_->min_length(), rhs_->min_length());
  }
  
  transition_.first = lhs_->transition().first;
  transition_.first.insert(rhs_->transition().first.begin(),
                           rhs_->transition().first.end());

  transition_.last = lhs_->transition().last;
  transition_.last.insert(rhs_->transition().last.begin(),
                          rhs_->transition().last.end());

  std::size_t id = info->xor_num++;
  lop_->set_id(id);
  rop_->set_id(id);
}

void XOR::FillTransition(bool reverse)
{
  rhs_->FillTransition(reverse);
  lhs_->FillTransition(reverse);
}

void Qmark::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);
  
  max_length_ = lhs_->min_length();
  min_length_ = 0;
  nullable_ = true;
  transition_.first = lhs_->transition().first;
  transition_.last = lhs_->transition().last;
  if (non_greedy_) NonGreedify();
}

void Qmark::FillTransition(bool reverse)
{
  lhs_->FillTransition(reverse);
}

void Star::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);

  max_length_ = std::numeric_limits<size_t>::max();
  min_length_ = 0;
  nullable_ = true;
  transition_.first = lhs_->transition().first;
  transition_.last = lhs_->transition().last;
  if (non_greedy_) NonGreedify();
}

void Star::FillTransition(bool reverse)
{
  Connect(lhs_->transition().last, lhs_->transition().first, reverse);
  lhs_->FillTransition(reverse);
}

void Plus::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);

  max_length_ = std::numeric_limits<size_t>::max();
  min_length_ = lhs_->min_length();
  nullable_ = lhs_->nullable();
  transition_.first = lhs_->transition().first;
  transition_.last = lhs_->transition().last;
}

void Plus::FillTransition(bool reverse)
{
  Connect(lhs_->transition().last, lhs_->transition().first, reverse);
  lhs_->FillTransition(reverse);
}

} // namespace regen
