#ifndef REGEN_EXPR_H_
#define REGEN_EXPR_H_

#include <list>
#include "util.h"

namespace regen {

class Expr;
class StateExpr;
class Literal; class CharClass; class Dot; class Anchor;
class None; class Epsilon; class Operator; class EOP;
class BinaryExpr;
class Concat; class Union; class Intersection; class XOR;
class UnaryExpr;
class Qmark; class Plus; class Star;
struct ExprPool;

class ExprVisitor {
public:
  virtual void Visit(Expr *) { exitmsg("bad Visitor implimentatin"); }
  virtual void Visit(StateExpr *e) { Visit((Expr*)e); }
  virtual void Visit(Literal *e) { Visit((StateExpr*)e); }
  virtual void Visit(CharClass *e) { Visit((StateExpr*)e); }
  virtual void Visit(Dot *e) { Visit((StateExpr*)e); }
  virtual void Visit(Anchor *e) { Visit((StateExpr*)e); }
  virtual void Visit(None *e) { Visit((StateExpr*)e); }
  virtual void Visit(Epsilon *e) { Visit((StateExpr*)e); }
  virtual void Visit(Operator *e) { Visit((StateExpr*)e); }
  virtual void Visit(EOP *e) { Visit((StateExpr*)e); }
  virtual void Visit(BinaryExpr *e) { Visit((Expr*)e); }
  virtual void Visit(Concat *e) { Visit((BinaryExpr*)e); }
  virtual void Visit(Union *e) { Visit((BinaryExpr*)e); }
  virtual void Visit(Intersection *e) { Visit((BinaryExpr*)e); }
  virtual void Visit(XOR *e) { Visit((BinaryExpr*)e); }
  virtual void Visit(UnaryExpr *e) { Visit((Expr*)e); }
  virtual void Visit(Qmark *e) { Visit((UnaryExpr*)e); }
  virtual void Visit(Plus *e) { Visit((UnaryExpr*)e); }
  virtual void Visit(Star *e) { Visit((UnaryExpr*)e); }
};

struct ExprInfo {
  ExprInfo(): xor_num(0), expr_root(NULL), orig_root(NULL), eop(NULL), min_length(0) {}
  std::size_t xor_num;
  Expr *expr_root;
  Expr *orig_root;
  EOP *eop;
  std::size_t min_length;
  std::bitset<256> involve;
};

struct Must {
  std::string *is;
  std::string *left;
  std::string *right;
  std::set<std::string> in;
};

struct Transition {
  std::set<StateExpr*> first;
  std::set<StateExpr*> last;
  std::set<StateExpr*> follow;
};

class Expr {
public:
  enum Type {
    kLiteral=0, kCharClass, kDot,
    kAnchor, kEOP, kOperator,
    kConcat, kUnion, kIntersection, kXOR,
    kQmark, kStar, kPlus,
    kEpsilon, kNone
  };
  enum SuperType {
    kStateExpr=0, kBinaryExpr, kUnaryExpr
  };
  enum GenOpt {
    GenRandom, GenLong, GenShort, GenAll
  };
  
  Expr(): parent_(NULL) {}
  virtual ~Expr() {}

  std::size_t max_length() { return max_length_; }
  void set_max_length(int len) { max_length_ = len; }
  std::size_t min_length() { return min_length_; }
  void set_min_length(int len) { min_length_ = len; }
  bool nullable() { return nullable_; }
  void set_nullable(bool b = true) { nullable_ = b; }
  bool extra() { return extra_; }
  void set_extra(bool b = true) { extra_ = b; }
  Transition& transition() { return transition_; }
  std::set<StateExpr*>& first() { return transition_.first; }
  std::set<StateExpr*>& last() { return transition_.last; }
  std::set<StateExpr*>& follow() { return transition_.follow; }

  Expr* parent() { return parent_; }
  void set_parent(Expr *parent) { parent_ = parent; }
  static const char* TypeString(Expr::Type type);
  static const char* SuperTypeString(Expr::SuperType stype);
  static SuperType SuperTypeOf(Expr *);

  virtual Expr::Type type() = 0;
  virtual Expr* Clone(ExprPool *) = 0;
  virtual void NonGreedify() = 0;
  virtual void FillPosition(ExprInfo *) = 0;
  virtual void FillTransition() = 0;
  virtual void Serialize(std::vector<Expr*> &v, ExprPool *p) { v.push_back(Clone(p)); }
  virtual void Factorize(std::vector<Expr*> &v) { v.push_back(this); }
  virtual void Generate(std::set<std::string> &g, GenOpt opt = GenAll, std::size_t n = 1) { g.insert(""); }
  virtual void PatchBackRef(Expr *, std::size_t, ExprPool *) = 0;
  static Expr* Shuffle(Expr *, Expr *, ExprPool *);
  static Expr* Permutation(Expr *, ExprPool *);
  
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
protected:
  static void Connect(std::set<StateExpr*> &src, std::set<StateExpr*> &dst);
  static void _Shuffle(std::vector<Expr*>&, std::size_t, std::vector<Expr*>&, std::size_t, std::vector<Expr*>&, Expr *c, ExprPool *);
  static void _Permutation(std::vector<Expr*> &, std::bitset<8> &, std::vector<Expr*> &, std::vector<std::size_t> &, ExprPool *);
  std::size_t max_length_;
  std::size_t min_length_;
  bool nullable_;
  bool extra_;
  Transition transition_;
  Expr *parent_;
private:
  DISALLOW_COPY_AND_ASSIGN(Expr);
};

struct ExprPool {
  typedef std::list<Expr *> Pool;
 public:
  ~ExprPool() { for(Pool::iterator i = pool.begin(); i != pool.end(); ++i) { delete *i; } }

  template<class T> T* alloc()
  { pool.push_back(0); Expr* &i = pool.back(); T* p = new T(); i = p; return p; }
  template<class T, class P1> T* alloc(P1 p1)
  { pool.push_back(0); Expr* &i = pool.back(); T* p = new T(p1); i = p; return p; }
  template<class T, class P1, class P2> T* alloc(P1 p1, P2 p2)
  { pool.push_back(0); Expr* &i = pool.back(); T* p = new T(p1, p2); i = p; return p; }
  template<class T, class P1, class P2, class P3> T* alloc(P1 p1, P2 p2, P3 p3)
  { pool.push_back(0); Expr* &i = pool.back(); T* p = new T(p1, p2, p3); i = p; return p; }

  void drain(ExprPool &p) { drain(&p); }
  void drain(ExprPool *p)
  {
    pool.insert(pool.end(), p->pool.begin(), p->pool.end());
    p->pool.clear();
  }

 private:
  Pool pool;
};

class StateExpr: public Expr {
public:
  StateExpr(): state_id_(0), non_greedy_(false)
  { max_length_ = min_length_ = 1; nullable_ = false; }
  ~StateExpr() {}
  void FillTransition() {}
  std::size_t state_id() { return state_id_; }
  void set_state_id(std::size_t id) { state_id_ = id; }
  bool non_greedy() { return non_greedy_; }
  void set_non_greedy(bool non_greedy = true) { non_greedy_ = non_greedy; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  void NonGreedify()  { non_greedy_ = true; }
  virtual bool Match(const unsigned char c) { return false; }
  void FillPosition(ExprInfo *) { transition_.first.insert(this); transition_.last.insert(this); }
  void PatchBackRef(Expr *, std::size_t, ExprPool *) {}
protected:
  std::size_t state_id_;
  bool non_greedy_;
  DISALLOW_COPY_AND_ASSIGN(StateExpr);
};

class Literal: public StateExpr {
public:
  Literal(const unsigned char literal): literal_(literal) {}
  ~Literal() {}
  unsigned char literal() { return literal_; }
  Expr::Type type() { return Expr::kLiteral; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(unsigned char c) { return c == literal_; };
  void FillPosition(ExprInfo *info) { transition_.first.insert(this); transition_.last.insert(this); info->involve.set(literal_); }
  Expr *Clone(ExprPool *p) { return p->alloc<Literal>(literal_); };
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n) { g.insert(std::string(1, literal_)); }
private:
  const unsigned char literal_;
  DISALLOW_COPY_AND_ASSIGN(Literal);
};

class CharClass: public StateExpr {
public:
  CharClass(): table_(std::bitset<256>()), negative_(false) {}
 CharClass(const std::bitset<256> &table, bool negative = false): table_(table), negative_(negative) {}
  CharClass(StateExpr *e1, StateExpr *e2);
  ~CharClass() {}
  std::bitset<256>& table() { return table_; }
  std::size_t count() const { return negative_ ? 256 - table_.count() : table_.count(); }
  void set_negative(bool negative) { negative_ = negative; }
  void flip() { table_.flip(); }
  bool negative() const { return negative_; }
  bool Involve(const unsigned char literal) const { return negative_ ? !table_[literal] : table_[literal]; }
  Expr::Type type() { return Expr::kCharClass; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(const unsigned char c) { return Involve(c); };
  void FillPosition(ExprInfo *info) { transition_.first.insert(this); transition_.last.insert(this); if (negative_) { std::bitset<256> table__ = table_; table__.flip(); info->involve |= table__; } else { info->involve |= table_; } }
  Expr *Clone(ExprPool *p) { return p->alloc<CharClass>(table_, negative_); };
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  std::bitset<256> table_;
  bool negative_;
  DISALLOW_COPY_AND_ASSIGN(CharClass);
};

class Dot: public StateExpr {
public:
  Dot() {}
  ~Dot() {}
  Expr::Type type() { return Expr::kDot; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(const unsigned char c) { return true; };
  void FillPosition(ExprInfo *info) { transition_.first.insert(this); transition_.last.insert(this); info->involve.set(); }
  Expr *Clone(ExprPool *p) { return p->alloc<Dot>(); };
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  DISALLOW_COPY_AND_ASSIGN(Dot);
};

/* Zero-width oprators */

class Anchor: public StateExpr {
public:
  enum Type { kBegLine, kEndLine };
  Anchor(Type type): atype_(type) { min_length_ = max_length_ = 0; }
  Expr::Type type() { return Expr::kAnchor; }
  Anchor::Type atype() { return atype_; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr *Clone(ExprPool *p) { return p->alloc<Anchor>(atype_); };
private:
  DISALLOW_COPY_AND_ASSIGN(Anchor);
  Type atype_;
};

class Operator: public StateExpr {
public:
  enum Type {
    kIntersection, kXOR, kBackRef
  };
  Operator(Type type): pair_(NULL), optype_(type), active_(false), id_(0) { min_length_ = max_length_ = 0; }
  Operator(Operator *o): pair_(NULL), optype_(o->optype()), active_(o->active()), id_(o->id()) { min_length_ = max_length_ = 0; }
  ~Operator() {}
  Expr::Type type() { return Expr::kOperator; }  
  void Accept(ExprVisitor* visit) { visit->Visit(this); }
  Expr *Clone(ExprPool *p) { return p->alloc<Operator>(this); };
  void PatchBackRef(Expr *, std::size_t, ExprPool *);
  Type optype() { return optype_; }
  void set_optype(Type t) { optype_ = t; }
  bool active() { return active_; }
  void set_active(bool b = true) { active_ = b; }
  Operator *pair() { return pair_; }
  void set_pair(Operator *p) { pair_ = p; }
  std::size_t id() { return id_; }
  void set_id(std::size_t id) { id_ = id; }
  static void NewPair(Operator **e1, Operator **e2, Type t, ExprPool *p)
  { *e1 = p->alloc<Operator>(t); *e2 = p->alloc<Operator>(t); (*e1)->set_pair(*e2); (*e2)->set_pair(*e1); }
private:
  Operator *pair_;
  Type optype_;
  bool active_;
  std::size_t id_;
};

class EOP: public StateExpr {
public:
  EOP() { min_length_ = max_length_ = 0; nullable_ = true; }
  ~EOP() {}
  Expr::Type type() { return Expr::kEOP; }  
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<EOP>(); };
private:
  DISALLOW_COPY_AND_ASSIGN(EOP);
};

class None: public StateExpr {
public:
  None() { min_length_ = max_length_ = 0; nullable_ = false; }
  ~None() {}
  Expr::Type type() { return Expr::kNone; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<None>(); };
private:
  DISALLOW_COPY_AND_ASSIGN(None);
};

class Epsilon: public StateExpr {
public:
  Epsilon() { min_length_ = max_length_ = 0; nullable_ = true; }
  ~Epsilon() {}
  Expr::Type type() { return Expr::kEpsilon; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<Epsilon>(); };
private:
  DISALLOW_COPY_AND_ASSIGN(Epsilon);
};

class BinaryExpr : public Expr {
public:
  BinaryExpr(Expr* lhs, Expr *rhs): lhs_(lhs), rhs_(rhs) { lhs->set_parent(this); rhs->set_parent(this); }
  ~BinaryExpr() {}
  Expr* lhs() { return lhs_; }
  void  set_lhs(Expr *lhs) { lhs_ = lhs; }
  Expr* rhs() { return rhs_; }
  void  set_rhs(Expr *rhs) { rhs_ = rhs; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  void NonGreedify()  { lhs_->NonGreedify(); rhs_->NonGreedify(); }
  void PatchBackRef(Expr *e, std::size_t i, ExprPool *p) { lhs_->PatchBackRef(e, i, p); rhs_->PatchBackRef(e, i, p); }
protected:
  Expr *lhs_;
  Expr *rhs_;
private:
  DISALLOW_COPY_AND_ASSIGN(BinaryExpr);
};

class Concat: public BinaryExpr {
public:
Concat(Expr *lhs, Expr *rhs, bool reverse = false): BinaryExpr(lhs, rhs) { if (reverse) std::swap(lhs_, rhs_); }
  ~Concat() {}
  void FillPosition(ExprInfo *);
  void FillTransition();
  Expr::Type type() { return Expr::kConcat; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<Concat>(lhs_->Clone(p), rhs_->Clone(p)); };
  void Serialize(std::vector<Expr*> &v, ExprPool *p);
  void Factorize(std::vector<Expr*> &v) { lhs_->Factorize(v); rhs_->Factorize(v); }
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  DISALLOW_COPY_AND_ASSIGN(Concat);
};

class Union: public BinaryExpr {
public:
  Union(Expr *lhs, Expr *rhs): BinaryExpr(lhs, rhs) {}
  ~Union() {}
  void FillPosition(ExprInfo *);
  void FillTransition();
  Expr::Type type() { return Expr::kUnion; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<Union>(lhs_->Clone(p), rhs_->Clone(p)); };
  void Serialize(std::vector<Expr*> &v, ExprPool *p);
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  DISALLOW_COPY_AND_ASSIGN(Union);
};

class Intersection: public BinaryExpr {
public:
  Intersection(Expr *lhs, Expr *rhs, ExprPool *p);
  ~Intersection() {}
  void FillPosition(ExprInfo *);
  void FillTransition();
  Expr::Type type() { return Expr::kIntersection; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<Intersection>(lhs__->Clone(p), rhs__->Clone(p), p); };
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  Operator *rop_, *lop_;
  Expr *lhs__, *rhs__;
  DISALLOW_COPY_AND_ASSIGN(Intersection);
};

class XOR: public BinaryExpr {
public:
  XOR(Expr *lhs, Expr *rhs, ExprPool *p);
  ~XOR() {}
  void FillPosition(ExprInfo *);
  void FillTransition();
  Expr::Type type() { return Expr::kXOR; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<XOR>(lhs__->Clone(p), rhs__->Clone(p), p); }
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  Operator *lop_, *rop_;
  Expr *lhs__, *rhs__;
  DISALLOW_COPY_AND_ASSIGN(XOR);
};

class UnaryExpr: public Expr {
public:
  UnaryExpr(Expr* lhs, double probability): lhs_(lhs), probability_(probability) { lhs->set_parent(this); }
  ~UnaryExpr() {}
  Expr* lhs() { return lhs_; }
  void  set_lhs(Expr *lhs) { lhs_ = lhs; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  void NonGreedify()  { lhs_->NonGreedify(); }
  void PatchBackRef(Expr *e, std::size_t i, ExprPool *p) { lhs_->PatchBackRef(e, i, p); }
  double probability() { return probability_; }
  void set_probability(double p) { probability_ = p; }
protected:
  Expr *lhs_;
  double probability_;
private:
  DISALLOW_COPY_AND_ASSIGN(UnaryExpr);
};

class Qmark: public UnaryExpr {
public:
  Qmark(Expr *lhs, bool non_greedy = false, double probability = 0.0): UnaryExpr(lhs, probability), non_greedy_(non_greedy) {}
  Qmark(Expr *lhs, double probability): UnaryExpr(lhs, probability), non_greedy_(false) {}
  ~Qmark() {}
  void FillPosition(ExprInfo *);
  void FillTransition();
  Expr::Type type() { return Expr::kQmark; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<Qmark>(lhs_->Clone(p), non_greedy_, probability_); };
  void Serialize(std::vector<Expr*> &v, ExprPool *p) { v.push_back(p->alloc<Epsilon>()); lhs_->Serialize(v, p); }
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  bool non_greedy_;
  DISALLOW_COPY_AND_ASSIGN(Qmark);
};

class Star: public UnaryExpr {
public:
  Star(Expr *lhs, bool non_greedy = false, double probability = 0.0): UnaryExpr(lhs, probability), non_greedy_(non_greedy) {}
  Star(Expr *lhs, double probability): UnaryExpr(lhs, probability), non_greedy_(false) {}
  ~Star() {}
  void FillPosition(ExprInfo *);
  void FillTransition();
  Expr::Type type() { return Expr::kStar; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool *p) { return p->alloc<Star>(lhs_->Clone(p), non_greedy_, probability_); };
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  bool non_greedy_;
  DISALLOW_COPY_AND_ASSIGN(Star);
};

class Plus: public UnaryExpr {
public:
  Plus(Expr *lhs, double probability = 0.0): UnaryExpr(lhs, probability) {}
  ~Plus() {}
  void FillPosition(ExprInfo *);
  void FillTransition();
  Expr::Type type() { return Expr::kPlus; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone(ExprPool* p) { return p->alloc<Plus>(lhs_->Clone(p), probability_); };
  void Generate(std::set<std::string> &g, GenOpt opt, std::size_t n);
private:
  DISALLOW_COPY_AND_ASSIGN(Plus);
};

} // namespace regen

#endif // REGEN_EXPR_H_
