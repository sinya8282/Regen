#include "expr.h"

namespace regen {

const char*
Expr::TypeString(Expr::Type type)
{
  static const char* const type_strings[] = {
    "Literal", "CharClass", "Dot",
    "BegLine", "EndLine", "EOP", "Operator",
    "Concat", "Union", "Intersection", "XOR",
    "Qmark", "Star", "Plus",
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

Expr::SuperType
Expr::SuperTypeOf(Expr *e)
{
  switch (e->type()) {
    case kLiteral: case kCharClass: case kDot:
    case kBegLine: case kEndLine: case kEOP: case kOperator:
    case kEpsilon: case kNone:
      return kStateExpr;
    case kConcat: case kUnion: case kIntersection: case kXOR:
      return kBinaryExpr;
    default: case kQmark: case kStar: case kPlus:
      return kUnaryExpr;
  }
}


void
Expr::Connect(std::set<StateExpr*> &src, std::set<StateExpr*> &dst, bool reverse)
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

Expr* Expr::Shuffle(Expr *lhs, Expr *rhs, ExprPool *p)
{
  Expr *e = NULL;
  std::vector<Expr *> ls, rs;
  ExprPool tmp_pool;
  lhs->Serialize(ls, &tmp_pool); rhs->Serialize(rs, &tmp_pool);

  for (std::vector<Expr*>::iterator i = ls.begin(); i != ls.end(); ++i) {
    for (std::vector<Expr*>::iterator j = rs.begin(); j != rs.end(); ++j) {
      std::vector<Expr*> lfac, rfac, result;

      (*i)->Factorize(lfac); (*j)->Factorize(rfac);
      _Shuffle(lfac, 0, rfac, 0, result, NULL, p);

      for (std::vector<Expr*>::iterator k = result.begin(); k != result.end(); ++k) {
        if (e == NULL) {
          e = *k;
        } else {
          e = p->alloc<Union>(e, *k);
        }
      }
    }
  }

  return e;
}

void Expr::_Shuffle(std::vector<Expr*>& lfac, std::size_t il, std::vector<Expr*>& rfac, std::size_t ir, std::vector<Expr*>& result, Expr *choice, ExprPool *p)
{
  if (lfac.size() <= il && rfac.size() <= ir) {
    result.push_back(choice);
    return;
  } else if (lfac.size() <= il) {
    Expr *e = rfac[ir]->Clone(p);
    for (std::size_t i = ir+1; i < rfac.size(); i++) {
      e = p->alloc<Concat>(e, rfac[i]->Clone(p));
    }
    result.push_back(p->alloc<Concat>(choice, e));
    return;
  } else if (rfac.size() <= ir) {
    Expr *e = lfac[il]->Clone(p);
    for (std::size_t i = il+1; i < lfac.size(); i++) {
      e = p->alloc<Concat>(e, lfac[i]->Clone(p));
    }
    result.push_back(p->alloc<Concat>(choice, e));
    return;
  }

  std::size_t il_ = il+1, ir_ = ir+1;
  if (choice == NULL) {
    _Shuffle(lfac, il_, rfac, ir, result, lfac[il]->Clone(p), p);
    _Shuffle(lfac, il, rfac, ir_, result, rfac[ir]->Clone(p), p);
  } else {
    _Shuffle(lfac, il_, rfac, ir, result, p->alloc<Concat>(choice->Clone(p), lfac[il]->Clone(p)), p);
    _Shuffle(lfac, il, rfac, ir_, result, p->alloc<Concat>(choice, rfac[ir]->Clone(p)), p);
  }
}

Expr* Expr::Permutation(Expr *e, ExprPool *p)
{
  std::vector<Expr*> es;
  ExprPool tmp_pool;
  e->Serialize(es, &tmp_pool);
  e = NULL;

  for (std::vector<Expr*>::iterator iter = es.begin(); iter != es.end(); ++iter) {
    std::vector<Expr*> fac;
    std::bitset<8> perm(false); //Maximum: 8! = 40320

    (*iter)->Factorize(fac);
    for (std::size_t i = 0; i < fac.size(); i++) {
      perm[i] = true;
    }
    std::vector<Expr*> result;
    std::vector<std::size_t> choice;
    _Permutation(fac, perm, result, choice, p);

    for (std::vector<Expr*>::iterator i = result.begin(); i != result.end(); ++i) {
      if (e == NULL) {
        e = *i;
      } else {
        e = p->alloc<Union>(e, *i);
      }
    }
  }
  
  return e;
}

void Expr::_Permutation(std::vector<Expr*> &v, std::bitset<8> &perm, std::vector<Expr*> &result, std::vector<std::size_t> &choice, ExprPool *p)
{
  if (perm.none()) {
    Expr *e = v[choice[0]]->Clone(p);
    for (std::size_t i = 1; i < choice.size(); i++) {
      e = p->alloc<Concat>(e, v[choice[i]]->Clone(p));
    }
    result.push_back(e);
    return;
  }

  for (std::size_t i = 0; i < perm.size(); i++) {
    if (perm[i]) {
      perm[i] = false; choice.push_back(i);
      _Permutation(v, perm, result, choice, p);
      perm[i] = true; choice.pop_back();
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
