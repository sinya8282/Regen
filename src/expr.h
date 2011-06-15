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
class EOP;
class BinaryExpr;
class Concat;
class Union;
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
  virtual void Visit(EOP *e) { Visit((StateExpr*)e); }
  virtual void Visit(BinaryExpr *e) { Visit((Expr*)e); }
  virtual void Visit(Concat *e) { Visit((BinaryExpr*)e); }
  virtual void Visit(Union *e) { Visit((BinaryExpr*)e); }
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
  std::set<StateExpr*> before;
  std::set<StateExpr*> follow;  
};

class Expr {
public:
  enum Type {
    kLiteral, kCharClass, kDot, kBegLine,
    kEndLine, kConcat, kUnion, kQmark,
    kStar, kPlus, kRpar, kLpar, kEpsilon,
    kNone, kNegative, kEOP
  };
  
  Expr(): parent_(NULL) {}
  virtual ~Expr() {}

  std::size_t expr_id() { return expr_id_; }
  void set_expr_id(std::size_t id) { expr_id_ = id; }
  std::size_t max_length() { return max_length_; }
  void set_max_length(int len) { max_length_ = len; }
  std::size_t min_length() { return min_length_; }
  void set_min_length(int len) { min_length_ = len; }
  Transition& transition() { return transition_; }
  Expr* parent() { return parent_; }
  void set_parent(Expr *parent) { parent_ = parent; }

  virtual Expr::Type type() = 0;
  virtual void FillTransition() = 0;
  
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
protected:
  static void Connect(std::set<StateExpr*> &src, std::set<StateExpr*> &dst);
  std::size_t expr_id_;
  std::size_t max_length_;
  std::size_t min_length_;
  Transition transition_;
private:
  Expr *parent_;
  DISALLOW_COPY_AND_ASSIGN(Expr);
};

class StateExpr: public Expr {
public:
  StateExpr();
  virtual ~StateExpr() {}
  void FillTransition() {}
  std::size_t state_id() { return state_id_; }
  void set_state_id(std::size_t id) { state_id_ = id; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  std::size_t state_id_;
  DISALLOW_COPY_AND_ASSIGN(StateExpr);
};

class Literal: public StateExpr {
public:
  Literal(const char literal): literal_(literal) {}
  ~Literal() {}
  char literal() { return literal_; }
  Expr::Type type() { return Expr::kLiteral; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  const char literal_;
  DISALLOW_COPY_AND_ASSIGN(Literal);
};

class CharClass: public StateExpr {
public:
  CharClass(): table_(std::bitset<256>()), negative_(false) { }
  CharClass(std::bitset<256> table): table_(table), negative_(false) {}
  ~CharClass() {}
  std::bitset<256>& table() { return table_; }
  std::size_t count() const { return negative_ ? 256 - count_ : count_; }
  void set_negative(bool negative) { negative_ = negative; }
  bool negative() const { return negative_; }
  bool Involve(const unsigned char literal) const { return negative_ ? !table_[literal] : table_[literal]; }
  Expr::Type type() { return Expr::kCharClass; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  std::bitset<256> table_;
  bool negative_;
  std::size_t count_;
  DISALLOW_COPY_AND_ASSIGN(CharClass);
};

class Dot: public StateExpr {
public:
  Dot() {}
  ~Dot() {}
  Expr::Type type() { return Expr::kDot; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(Dot);
};

class BegLine: public StateExpr {
public:
  BegLine() {}
  ~BegLine() {}
  Expr::Type type() { return Expr::kBegLine; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(BegLine);
};

class EndLine: public StateExpr {
public:
  EndLine() {}
  ~EndLine() {}
  Expr::Type type() { return Expr::kEndLine; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(EndLine);
};

class EOP: public StateExpr {
public:
  EOP() { min_length_ = max_length_ = 0; }
  ~EOP() {}
  Expr::Type type() { return Expr::kEOP; }  
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(EOP);
};

class None: public StateExpr {
public:
  None() { min_length_ = max_length_ = 0; }
  ~None() {}
  Expr::Type type() { return Expr::kNone; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(None);
};

class Epsilon: public StateExpr {
public:
  Epsilon() { min_length_ = max_length_ = 0; }
  ~Epsilon() {}
  Expr::Type type() { return Expr::kEpsilon; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(Epsilon);
};

class BinaryExpr : public Expr {
public:
  BinaryExpr(Expr* lhs, Expr *rhs): lhs_(lhs), rhs_(rhs) {}
  virtual ~BinaryExpr() { delete lhs_; delete rhs_; }
  Expr* lhs() { return lhs_; }
  void  set_lhs(Expr *lhs) { lhs_ = lhs; }
  Expr* rhs() { return rhs_; }
  void  set_rhs(Expr *rhs) { rhs_ = rhs; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
protected:
  Expr *lhs_;
  Expr *rhs_;
private:
  DISALLOW_COPY_AND_ASSIGN(BinaryExpr);
};

class Concat: public BinaryExpr {
public:
  Concat(Expr *lhs, Expr *rhs);
  ~Concat() {}
  void FillTransition();
  Expr::Type type() { return Expr::kConcat; }  
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(Concat);
};

class Union: public BinaryExpr {
public:
  Union(Expr *lhs, Expr *rhs);
  ~Union() {}
  void FillTransition();
  Expr::Type type() { return Expr::kUnion; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(Union);
};

class UnaryExpr: public Expr {
public:
  UnaryExpr(Expr* lhs): lhs_(lhs) {}
  virtual ~UnaryExpr() { delete lhs_; }
  Expr* lhs() { return lhs_; }
  void  set_lhs(Expr *lhs) { lhs_ = lhs; }
  static UnaryExpr* DispatchNew(Expr::Type type, Expr* lhs);
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
protected:
  Expr *lhs_;
private:
  DISALLOW_COPY_AND_ASSIGN(UnaryExpr);
};

class Qmark: public UnaryExpr {
public:
  Qmark(Expr *lhs);
  ~Qmark() {}
  void FillTransition();
  Expr::Type type() { return Expr::kQmark; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(Qmark);
};

class Star: public UnaryExpr {
public:
  Star(Expr *lhs);
  ~Star() {}
  void FillTransition();
  Expr::Type type() { return Expr::kStar; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(Star);
};

class Plus: public UnaryExpr {
public:
  Plus(Expr *lhs);
  ~Plus() {}
  void FillTransition();
  Expr::Type type() { return Expr::kPlus; }
  virtual void Accept(ExprVisitor* visit) { visit->Visit(this); };
private:
  DISALLOW_COPY_AND_ASSIGN(Plus);
};

} // namespace regen

#endif // REGEN_EXPR_H_
