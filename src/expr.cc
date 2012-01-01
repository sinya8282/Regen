#include "expr.h"

namespace regen {

const char* Expr::TypeString(Expr::Type type)
{
  static const char* const type_strings[] = {
    "Literal", "CharClass", "Dot",
    "Anchor", "EOP", "Operator",
    "Concat", "Union", "Intersection", "XOR",
    "Qmark", "Star", "Plus",
    "Epsilon", "None"
  };

  return type_strings[type];
}

const char* Expr::SuperTypeString(Expr::SuperType stype)
{
  static const char* const stype_strings[] = {
    "StateExpr", "BinaryExpr", "UnaryExpr"
  };

  return stype_strings[stype];
}

Expr::SuperType Expr::SuperTypeOf(Expr *e)
{
  switch (e->type()) {
    case kLiteral: case kCharClass: case kDot:
    case kAnchor: case kEOP: case kOperator:
    case kEpsilon: case kNone:
      return kStateExpr;
    case kConcat: case kUnion: case kIntersection: case kXOR:
      return kBinaryExpr;
    default: case kQmark: case kStar: case kPlus:
      return kUnaryExpr;
  }
}

void Expr::Connect(std::set<StateExpr*> &src, std::set<StateExpr*> &dst)
{
  for (std::set<StateExpr*>::iterator iter = src.begin(); iter != src.end(); ++iter) {
    (*iter)->follow().insert(dst.begin(), dst.end());
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

class Shortp {
public:
  bool operator()( const std::string& lhs, const std::string& rhs ) const
  {
    return lhs.size() < rhs.size();
  }
};

class Longp {
public:
  bool operator()( const std::string& lhs, const std::string& rhs ) const
  {
    return lhs.size() >= rhs.size();
  }
};

void Trim(std::set<std::string> &g, Expr::GenOpt opt, std::size_t n)
{
  if (opt == Expr::GenAll || g.size() <= n) return;
  std::vector<std::string> v(g.size());
  std::copy(g.begin(), g.end(), v.begin());
  switch (opt) {
    case Expr::GenRandom:
      random_shuffle(v.begin(), v.end());
      break;
    case Expr::GenLong:
      std::sort(v.end(), v.begin(), Longp());
      break;
    case Expr::GenShort:
      std::sort(v.end(), v.begin(), Shortp());
      break;
    default:
      break;
  }
  v.resize(n);
  g.clear();
  g.insert(v.begin(), v.end());
}

void CharClass::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  for (std::size_t i = 32; i < 127; i++) {
    if (Involve((unsigned char)i)) {
      g.insert(std::string(1, (char)i));
    }
  }
  Trim(g, opt, n);
}

void Dot::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  for (std::size_t i = 32; i < 127; i++){
    g.insert(std::string(1, (char)i));
  }
  Trim(g, opt, n);
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

  first() = lhs_->first();

  if (lhs_->nullable()) {
    first().insert(rhs_->first().begin(), rhs_->first().end());
  }

  last() = rhs_->last();

  if (rhs_->nullable()) {
    last().insert(lhs_->last().begin(), lhs_->last().end());
  }
}

void Concat::FillTransition()
{
  Connect(lhs_->last(), rhs_->first());
  rhs_->FillTransition();
  lhs_->FillTransition();
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

void Concat::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  std::set<std::string> h, r;
  lhs_->Generate(g);
  rhs_->Generate(h);
  for (std::set<std::string>::iterator i = g.begin(); i != g.end(); ++i) {
    for (std::set<std::string>::iterator j = h.begin(); j != h.end(); ++j) {
      r.insert(*i+*j);
    }
  }
  g.swap(r);
  Trim(g, opt, n);
}

void Union::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);
  rhs_->FillPosition(info);
  
  max_length_ = std::max(lhs_->max_length(), rhs_->max_length());
  min_length_ = std::min(lhs_->min_length(), rhs_->min_length());
  nullable_ = lhs_->nullable() || rhs_->nullable();

  first() = lhs_->first();
  first().insert(rhs_->first().begin(), rhs_->first().end());

  last() = lhs_->last();
  last().insert(rhs_->last().begin(), rhs_->last().end());
}

void Union::FillTransition()
{
  rhs_->FillTransition();
  lhs_->FillTransition();
}

void Union::Serialize(std::vector<Expr*> &v, ExprPool *p)
{
  lhs_->Serialize(v, p);
  rhs_->Serialize(v, p);
}

void Union::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  std::set<std::string> h;
  lhs_->Generate(g);
  rhs_->Generate(h);
  std::vector<std::string> r(g.size()+h.size());
  std::set_union(g.begin(), g.end(), h.begin(), h.end(), r.begin());
  if (g.find("") == g.end() && h.find("") == h.end()) {
    r.erase(std::remove(r.begin(), r.end(), ""), r.end());
  }    
  g.clear();
  g.insert(r.begin(), r.end());
  Trim(g, opt, n);
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

  first() = first();
  first().insert(rhs_->first().begin(), rhs_->first().end());

  last() = lhs_->last();
  last().insert(rhs_->last().begin(), rhs_->last().end());
}

void Intersection::FillTransition()
{
  rhs_->FillTransition();
  lhs_->FillTransition();
}

void Intersection::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  std::set<std::string> h;
  lhs_->Generate(g);
  rhs_->Generate(h);
  std::vector<std::string> r(g.size()+h.size());
  std::set_intersection(g.begin(), g.end(), h.begin(), h.end(), r.begin());
  if (g.find("") == g.end() || h.find("") == h.end()) {
    r.erase(std::remove(r.begin(), r.end(), ""), r.end());
  }    
  g.clear();
  g.insert(r.begin(), r.end());
  Trim(g, opt, n);
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
  
  first() = lhs_->first();
  first().insert(rhs_->first().begin(), rhs_->first().end());

  last() = lhs_->last();
  last().insert(rhs_->last().begin(), rhs_->last().end());

  std::size_t id = info->xor_num++;
  lop_->set_id(id);
  rop_->set_id(id);
}

void XOR::FillTransition()
{
  rhs_->FillTransition();
  lhs_->FillTransition();
}

void XOR::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  std::set<std::string> h;
  lhs_->Generate(g);
  rhs_->Generate(h);
  std::vector<std::string> r1(g.size());
  std::vector<std::string> r2(h.size());
  std::set_difference(g.begin(), g.end(), h.begin(), h.end(), r1.begin());
  std::set_difference(h.begin(), h.end(), g.begin(), g.end(), r2.begin());
  if ((g.find("") == g.end()) == ( h.find("") == h.end())) {
    r1.erase(std::remove(r1.begin(), r1.end(), ""), r1.end());
    r2.erase(std::remove(r2.begin(), r2.end(), ""), r2.end());
  }    
  g.clear();
  g.insert(r1.begin(), r1.end());
  g.insert(r2.begin(), r2.end());
  Trim(g, opt, n);
}

void Qmark::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);
  
  max_length_ = lhs_->min_length();
  min_length_ = 0;
  nullable_ = true;
  first() = lhs_->first();
  last() = lhs_->last();
  if (non_greedy_) NonGreedify();
}

void Qmark::FillTransition()
{
  lhs_->FillTransition();
}

double frand()
{
  double f = (double)rand() / RAND_MAX;
  return f * 100.0;
}

void Qmark::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  if (probability_ != 0.0) {
    if (frand() < probability_) {
      lhs_->Generate(g);
    } else {
      g.insert("");
    }
  } else {
    lhs_->Generate(g);
    g.insert("");
  }
  Trim(g, opt, n);
}

void Star::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);

  max_length_ = std::numeric_limits<size_t>::max();
  min_length_ = 0;
  nullable_ = true;
  first() = lhs_->first();
  last() = lhs_->last();
  if (non_greedy_) NonGreedify();
}

void Star::FillTransition()
{
  Connect(lhs_->last(), lhs_->first());
  lhs_->FillTransition();
}

void Star::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  if (probability_ != 0.0 && frand() < probability_) {
    lhs_->Generate(g);
    std::set<std::string> h(g), r;
    while (frand() < probability_) {
      r.clear();
      for (std::set<std::string>::iterator i = g.begin(); i != g.end(); ++i) {
        for (std::set<std::string>::iterator j = h.begin(); j != h.end(); ++j) {
          r.insert(*i+*j);
        }
      }
      g.swap(r);
    }
  } else {
    g.insert("");
  }
  Trim(g, opt, n);
}

void Plus::FillPosition(ExprInfo *info)
{
  lhs_->FillPosition(info);

  max_length_ = std::numeric_limits<size_t>::max();
  min_length_ = lhs_->min_length();
  nullable_ = lhs_->nullable();
  first() = lhs_->first();
  last() = lhs_->last();
}

void Plus::FillTransition()
{
  Connect(lhs_->last(), lhs_->first());
  lhs_->FillTransition();
}

void Plus::Generate(std::set<std::string> &g, GenOpt opt, std::size_t n)
{
  lhs_->Generate(g);
  if (probability_ != 0.0) {
    std::set<std::string> h(g), r;
    while (frand() < probability_) {
      r.clear();
      for (std::set<std::string>::iterator i = g.begin(); i != g.end(); ++i) {
        for (std::set<std::string>::iterator j = h.begin(); j != h.end(); ++j) {
          r.insert(*i+*j);
        }
      }
      g.swap(r);
    }
  }
  Trim(g, opt, n);
}

} // namespace regen
