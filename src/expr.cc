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

void Expr::Shuffle(Expr *lhs, Expr *rhs, std::vector<Expr *> &result, ExprPool *p)
{
  std::vector<Expr*> l, r;
  lhs->Factorize(l); rhs->Factorize(r);
  _Shuffle(l, 0, r, 0, result, NULL, p);
}

void Expr::_Shuffle(std::vector<Expr*>& lhs, std::size_t il, std::vector<Expr*>& rhs, std::size_t ir, std::vector<Expr*>& result, Expr *choice, ExprPool *p)
{
  if (lhs.size() <= il && rhs.size() <= ir) {
    result.push_back(choice);
    return;
  } else if (lhs.size() <= il) {
    Expr *e = rhs[ir]->Clone(p);
    for (std::size_t i = ir+1; i < rhs.size(); i++) {
      e = p->alloc<Concat>(e, rhs[i]->Clone(p));
    }
    result.push_back(p->alloc<Concat>(choice, e));
    return;
  } else if (rhs.size() <= ir) {
    Expr *e = lhs[il]->Clone(p);
    for (std::size_t i = il+1; i < lhs.size(); i++) {
      e = p->alloc<Concat>(e, lhs[i]->Clone(p));
    }
    result.push_back(p->alloc<Concat>(choice, e));
    return;
  }

  std::size_t il_ = il+1, ir_ = ir+1;
  if (choice == NULL) {
    _Shuffle(lhs, il_, rhs, ir, result, lhs[il]->Clone(p), p);
    _Shuffle(lhs, il, rhs, ir_, result, rhs[ir]->Clone(p), p);
  } else {
    _Shuffle(lhs, il_, rhs, ir, result, p->alloc<Concat>(choice->Clone(p), lhs[il]->Clone(p)), p);
    _Shuffle(lhs, il, rhs, ir_, result, p->alloc<Concat>(choice, rhs[ir]->Clone(p)), p);
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

void Operator::PatchBackRef(Expr *patch, std::size_t i, ExprPool *p)
{
  if (optype_ == kBackRef && id_ == i) {
    if (!active_) patch = patch->Clone(p);
    patch->set_parent(parent_);
    switch (parent_->type()) {
      case Expr::kConcat: case Expr::kUnion:
      case Expr::kIntersection: case Expr::kXOR: {
        BinaryExpr* p = static_cast<BinaryExpr*>(parent_);
        if (p->lhs() == this) {
          p->set_lhs(patch);
        } else if (p->rhs() == this) {
          p->set_rhs(patch);
        } else {
          exitmsg("inconsistency parent-child pointer");
        }
        break;
      }
      case Expr::kQmark: case Expr::kStar: case Expr::kPlus: {
        UnaryExpr *p = static_cast<UnaryExpr*>(parent_);
        if (p->lhs() == this) {
          p->set_lhs(patch);
        } else {
          exitmsg("inconsistency parent-child pointer");
        }
        break;
      }
      default: exitmsg("invalid types");
    }
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

void Concat::Serialize(std::vector<Expr*> &v, ExprPool *p)
{
  std::size_t ini = v.size();
  lhs_->Serialize(v, p);
  std::size_t lnum = v.size()-ini;
  rhs_->Serialize(v, p);
  std::size_t rnum = v.size()-lnum-ini;
  std::vector<Expr*> v_(lnum+rnum);
  std::copy(v.begin()+ini, v.end(), v_.begin());
  v.resize(ini+lnum*rnum);
  Expr *lhs, *rhs;
  for (std::size_t li = 0; li < lnum; li++) {
    for (std::size_t ri = 0; ri < rnum; ri++) {
      lhs = ri == 0 ? v_[li] : v_[li]->Clone(p);
      rhs = li == 0 ? v_[ri+lnum] : v_[ri+lnum]->Clone(p);
      v[ri+li*rnum+ini] = p->alloc<Concat>(lhs, rhs);
    }
  }
  return;
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

void Union::Serialize(std::vector<Expr*> &v, ExprPool *p)
{
  lhs_->Serialize(v, p);
  rhs_->Serialize(v, p);
}

Intersection::Intersection(Expr *lhs, Expr *rhs, ExprPool *p):
    BinaryExpr(lhs, rhs), lhs__(lhs), rhs__(rhs)
{
  Operator::NewPair(&lop_, &rop_, Operator::kIntersection, p);
  lhs_ = p->alloc<Concat>(lhs_, lop_);
  rhs_ = p->alloc<Concat>(rhs_, rop_);
  lhs_->set_parent(this);
  rhs_->set_parent(this);
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

XOR::XOR(Expr* lhs, Expr* rhs, ExprPool *p):
    BinaryExpr(lhs, rhs), lhs__(lhs), rhs__(rhs)
{
  Operator::NewPair(&lop_, &rop_, Operator::kXOR, p);
  lhs_ = p->alloc<Concat>(lhs, lop_);
  rhs_ = p->alloc<Concat>(rhs, rop_);
  lhs_->set_parent(this);
  rhs_->set_parent(this);
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
