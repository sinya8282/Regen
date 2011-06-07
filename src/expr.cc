#include "expr.h"

namespace regen {

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

StateExpr::StateExpr(Expr::Type type):
    type_(type),
    state_id_(0)
{
  min_length_ = max_length_ = 1;
  transition_.first.insert(this);
  transition_.last.insert(this);
}

UnaryExpr* UnaryExpr::DispatchNew(Expr::Type type, Expr* lhs)
{
  UnaryExpr *ret;
  switch (type) {
    case Expr::kQmark:
      ret = new regen::Qmark(lhs);
      break;
    case Expr::kStar:
      ret = new regen::Star(lhs);
      break;
    case Expr::kPlus:
      ret = new regen::Plus(lhs);
      break;
    default:
      exitmsg("invalid Expr type: %d", type);
  }
  return ret;
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
