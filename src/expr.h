#ifndef REGEN_EXPR_H_
#define REGEN_EXPR_H_

#include "util.h"

namespace regen {

class Expr;
class StateExpr;
class Literal;
class CharClass;
class Dot;
class BegLine;
class EndLine;
class None;
class Epsilon;
class Operator;
class EOP;
class BinaryExpr;
class Concat;
class Union;
class Intersection;
class XOR;
class UnaryExpr;
class Qmark;
class Plus;
class Star;

class ExprVisitor {
public:
  virtual void Visit(Expr *) { exitmsg("bad Visitor implimentatin"); }
  virtual void Visit(StateExpr *e) { Visit((Expr*)e); }
  virtual void Visit(Literal *e) { Visit((StateExpr*)e); }
  virtual void Visit(CharClass *e) { Visit((StateExpr*)e); }
  virtual void Visit(Dot *e) { Visit((StateExpr*)e); }
  virtual void Visit(BegLine *e) { Visit((StateExpr*)e); }
  virtual void Visit(EndLine *e) { Visit((StateExpr*)e); }
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

struct ExprInfo {
  ExprInfo(): xor_num(0) {}
  std::size_t xor_num;
};

class Expr {
public:
  enum Type {
    kLiteral=0, kCharClass, kDot,
    kBegLine, kEndLine, kEOP, kOperator,
    kConcat, kUnion, kIntersection, kXOR,
    kQmark, kStar, kPlus,
    kEpsilon, kNone
  };
  enum SuperType {
    kStateExpr=0, kBinaryExpr, kUnaryExpr
  };

  Expr(): parent_(NULL) {}
  virtual ~Expr() {}

  std::size_t expr_id() { return expr_id_; }
  void set_expr_id(std::size_t id) { expr_id_ = id; }
  std::size_t max_length() { return max_length_; }
  void set_max_length(int len) { max_length_ = len; }
  std::size_t min_length() { return min_length_; }
  void set_min_length(int len) { min_length_ = len; }
  bool nullable() { return nullable_; }
  void set_nullable(bool b = true) { nullable_ = b; }
  Transition& transition() { return transition_; }

  Expr* parent() { return parent_; }
  void set_parent(Expr *parent) { parent_ = parent; }
  static const char* TypeString(Expr::Type type);
  static const char* SuperTypeString(Expr::SuperType stype);

  virtual Expr::Type type() = 0;
  virtual Expr::SuperType stype() = 0;
  virtual Expr* Clone() = 0;
  virtual void NonGreedify() = 0;
  virtual void FillPosition(ExprInfo *) = 0;
  virtual void FillTransition(bool reverse = false) = 0;
  void FillReverseTransition() { FillTransition(true); transition_.first.swap(transition_.last); }
  
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
protected:
  static void Connect(std::set<StateExpr*> &src, std::set<StateExpr*> &dst, bool reverse = false);
  std::size_t expr_id_;
  std::size_t max_length_;
  std::size_t min_length_;
  bool nullable_;
  Transition transition_;
private:
  Expr *parent_;
  DISALLOW_COPY_AND_ASSIGN(Expr);
};

class StateExpr: public Expr {
public:
  StateExpr(): state_id_(0), non_greedy_(false), non_greedy_pair_(NULL)
  { max_length_ = min_length_ = 1; nullable_ = false; }
  ~StateExpr() {}
  void FillTransition(bool reverse = false) {}
  std::size_t state_id() { return state_id_; }
  void set_state_id(std::size_t id) { state_id_ = id; }
  bool non_greedy() { return non_greedy_; }
  StateExpr* non_greedy_pair() { return non_greedy_pair_; }
  void set_non_greedy_pair(StateExpr* p) { non_greedy_pair_ = p; }
  void set_non_greedy(bool non_greedy = true) { non_greedy_ = non_greedy; }
  Expr::SuperType stype() { return Expr::kStateExpr; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  void NonGreedify()  { non_greedy_ = true; }
  void FillPosition(ExprInfo *) { transition_.first.insert(this); transition_.last.insert(this); }
  virtual bool Match(const unsigned char c) = 0;
private:
  std::size_t state_id_;
  bool non_greedy_;
  StateExpr* non_greedy_pair_;
  DISALLOW_COPY_AND_ASSIGN(StateExpr);
};

class Literal: public StateExpr {
public:
  Literal(const unsigned char literal): literal_(literal) {}
  ~Literal() {}
  unsigned char literal() { return literal_; }
  Expr::Type type() { return Expr::kLiteral; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(const unsigned char c) { return c == literal_; };
  Expr *Clone() { return new Literal(literal_); };
private:
  const unsigned char literal_;
  DISALLOW_COPY_AND_ASSIGN(Literal);
};

class CharClass: public StateExpr {
public:
  CharClass(): table_(std::bitset<256>()), negative_(false) {}
  CharClass(std::bitset<256> &table): table_(table), negative_(false) {}
  CharClass(std::bitset<256> &table, bool negative): table_(table), negative_(negative) {}
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
  Expr *Clone() { return new CharClass(table_, negative_); };
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
  Expr *Clone() { return new Dot(); };
private:
  DISALLOW_COPY_AND_ASSIGN(Dot);
};

class BegLine: public StateExpr {
public:
  BegLine() {}
  ~BegLine() {}
  Expr::Type type() { return Expr::kBegLine; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(const unsigned char c) { return c == '\n'; };
  Expr* Clone() { return new BegLine(); };
private:
  DISALLOW_COPY_AND_ASSIGN(BegLine);
};

class EndLine: public StateExpr {
public:
  EndLine() {}
  ~EndLine() {}
  Expr::Type type() { return Expr::kEndLine; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(const unsigned char c) { return c == '\n'; };
  Expr* Clone() { return new EndLine(); };
private:
  DISALLOW_COPY_AND_ASSIGN(EndLine);
};

class Operator: public StateExpr {
public:
  Operator(): pair_(NULL), active_(false), id_(0) { min_length_ = max_length_ = 0; }
  ~Operator() {}
  enum Type {
    kIntersection, kXOR
  };
  Expr::Type type() { return Expr::kOperator; }  
  void Accept(ExprVisitor* visit) { visit->Visit(this); }
  bool Match(const unsigned char c) { return false; }
  Expr* Clone() { return new Operator(); }
  Type optype() { return optype_; }
  void set_optype(Type t) { optype_ = t; }
  bool active() { return active_; }
  void set_active(bool b = true) { active_ = b; }
  Operator *pair() { return pair_; }
  void set_pair(Operator *p) { pair_ = p; }
  std::size_t id() { return id_; }
  void set_id(std::size_t id) { id_ = id; }
  static void NewPair(Operator **e1, Operator **e2, Type t = kXOR)
  { *e1 = new Operator(); *e2 = new Operator(); (*e1)->set_pair(*e2); (*e2)->set_pair(*e1); (*e1)->set_optype(t); (*e2)->set_optype(t); }
private:
  Operator *pair_;
  Type optype_;
  bool active_;
  std::size_t id_;
  DISALLOW_COPY_AND_ASSIGN(Operator);
};

class EOP: public StateExpr {
public:
  EOP() { min_length_ = max_length_ = 0; nullable_ = true; }
  ~EOP() {}
  Expr::Type type() { return Expr::kEOP; }  
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(const unsigned char c) { return false; };
  Expr* Clone() { return new EOP(); };
private:
  DISALLOW_COPY_AND_ASSIGN(EOP);
};

class None: public StateExpr {
public:
  None() { min_length_ = max_length_ = 0; nullable_ = true; }
  ~None() {}
  Expr::Type type() { return Expr::kNone; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(const unsigned char c) { return false; };
  Expr* Clone() { return new None(); };
private:
  DISALLOW_COPY_AND_ASSIGN(None);
};

class Epsilon: public StateExpr {
public:
  Epsilon() { min_length_ = max_length_ = 0; nullable_ = true; }
  ~Epsilon() {}
  Expr::Type type() { return Expr::kEpsilon; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  bool Match(const unsigned char c) { return true; };
  Expr* Clone() { return new Epsilon(); };    
private:
  DISALLOW_COPY_AND_ASSIGN(Epsilon);
};

class BinaryExpr : public Expr {
public:
  BinaryExpr(Expr* lhs, Expr *rhs): lhs_(lhs), rhs_(rhs) {}
  ~BinaryExpr() { delete lhs_; delete rhs_; }
  Expr* lhs() { return lhs_; }
  void  set_lhs(Expr *lhs) { lhs_ = lhs; }
  Expr* rhs() { return rhs_; }
  void  set_rhs(Expr *rhs) { rhs_ = rhs; }
  Expr::SuperType stype() { return Expr::kBinaryExpr; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  void NonGreedify()  { lhs_->NonGreedify(); rhs_->NonGreedify(); }
protected:
  Expr *lhs_;
  Expr *rhs_;
private:
  DISALLOW_COPY_AND_ASSIGN(BinaryExpr);
};

class Concat: public BinaryExpr {
public:
  Concat(Expr *lhs, Expr *rhs): BinaryExpr(lhs, rhs) {}
  ~Concat() {}
  void FillPosition(ExprInfo *);
  void FillTransition(bool reverse = false);
  Expr::Type type() { return Expr::kConcat; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone() { return new Concat(lhs_->Clone(), rhs_->Clone()); };
private:
  DISALLOW_COPY_AND_ASSIGN(Concat);
};

class Union: public BinaryExpr {
public:
  Union(Expr *lhs, Expr *rhs): BinaryExpr(lhs, rhs) {}
  ~Union() {}
  void FillPosition(ExprInfo *);
  void FillTransition(bool reverse = false);
  Expr::Type type() { return Expr::kUnion; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone() { return new Union(lhs_->Clone(), rhs_->Clone()); };
private:
  DISALLOW_COPY_AND_ASSIGN(Union);
};

class Intersection: public BinaryExpr {
public:
  Intersection(Expr *lhs, Expr *rhs);
  ~Intersection() {}
  void FillPosition(ExprInfo *);
  void FillTransition(bool reverse = false);
  Expr::Type type() { return Expr::kIntersection; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone() { return new Intersection(lhs_->Clone(), rhs_->Clone()); };
private:
  Operator *op1_, *op2_;
  Expr *lhs__, *rhs__;
  DISALLOW_COPY_AND_ASSIGN(Intersection);
};

class XOR: public BinaryExpr {
public:
  XOR(Expr *lhs, Expr *rhs);
  ~XOR() {}
  void FillPosition(ExprInfo *);
  void FillTransition(bool reverse = false);
  Expr::Type type() { return Expr::kXOR; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone() { return new XOR(lhs_->Clone(), rhs_->Clone()); };
private:
  Operator *op1_, *op2_;
  Expr *lhs__, *rhs__;
  DISALLOW_COPY_AND_ASSIGN(XOR);
};

class UnaryExpr: public Expr {
public:
  UnaryExpr(Expr* lhs): lhs_(lhs) {}
  ~UnaryExpr() { delete lhs_; }
  Expr* lhs() { return lhs_; }
  void  set_lhs(Expr *lhs) { lhs_ = lhs; }
  Expr::SuperType stype() { return Expr::kUnaryExpr; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  void NonGreedify()  { lhs_->NonGreedify(); }
protected:
  Expr *lhs_;
private:
  DISALLOW_COPY_AND_ASSIGN(UnaryExpr);
};

class Qmark: public UnaryExpr {
public:
  Qmark(Expr *lhs, bool non_greedy = false): UnaryExpr(lhs), non_greedy_(non_greedy) {}
  ~Qmark() {}
  void FillPosition(ExprInfo *);
  void FillTransition(bool reverse = false);
  Expr::Type type() { return Expr::kQmark; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone() { return new Qmark(lhs_->Clone()); };
private:
  bool non_greedy_;
  DISALLOW_COPY_AND_ASSIGN(Qmark);
};

class Star: public UnaryExpr {
public:
  Star(Expr *lhs, bool non_greedy = false): UnaryExpr(lhs), non_greedy_(non_greedy) {}
  ~Star() {}
  void FillPosition(ExprInfo *);
  void FillTransition(bool reverse = false);
  Expr::Type type() { return Expr::kStar; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone() { return new Star(lhs_->Clone()); };
private:
  bool non_greedy_;
  DISALLOW_COPY_AND_ASSIGN(Star);
};

class Plus: public UnaryExpr {
public:
  Plus(Expr *lhs): UnaryExpr(lhs) {}
  ~Plus() {}
  void FillPosition(ExprInfo *);
  void FillTransition(bool reverse = false);
  Expr::Type type() { return Expr::kPlus; }
  void Accept(ExprVisitor* visit) { visit->Visit(this); };
  Expr* Clone() { return new Plus(lhs_->Clone()); };
private:
  DISALLOW_COPY_AND_ASSIGN(Plus);
};

} // namespace regen

#endif // REGEN_EXPR_H_
