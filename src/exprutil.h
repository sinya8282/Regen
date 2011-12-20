#ifndef REGEN_EXPRUTIL_H_
#define REGEN_EXPRUTIL_H_

#include "expr.h"

namespace regen {

class NumberingStateExprVisitor: public ExprVisitor {
public:
  void Visit(Expr*) {}
  void Visit(StateExpr *e) { state_exprs_->push_back(e); e->set_state_id(state_id_++); }
  void Visit(BinaryExpr *e) { e->lhs()->Accept(this); e->rhs()->Accept(this); }
  void Visit(UnaryExpr *e) { e->lhs()->Accept(this); }
  static void Numbering(Expr *e, std::vector<StateExpr*> *state_exprs);
 private:
  std::size_t state_id_;
  std::vector<StateExpr*> *state_exprs_;
};

  
class PrintExprVisitor: public ExprVisitor {
public:
  void Visit(Expr*) {}
  void Visit(Literal* e);
  void Visit(CharClass* e);
  void Visit(Dot* e) { printf("."); }
  void Visit(BegLine* e) { printf("^"); }
  void Visit(EndLine* e) { printf("$"); }
  void Visit(None * e) { printf("[:None:]"); }
  void Visit(Concat* e) {}
  void Visit(Union* e) { printf("|"); }
  void Visit(Intersection* e) { printf("&"); }
  void Visit(XOR* e) { printf("&&"); }
  void Visit(Qmark* e) { printf("?"); }
  void Visit(Plus* e) { printf("+"); }
  void Visit(Star* e) { printf("*"); }
  void Visit(EOP* e) {}
  static void Print(Expr *e);
protected:
  PrintExprVisitor() {}
};
  
class PrintRegexVisitor: public ExprVisitor {
public:
  void Visit(StateExpr *e) { PrintExprVisitor::Print(e); }
  void Visit(BinaryExpr *e);
  void Visit(UnaryExpr *e);
  static void Print(Expr *e);
private:
  PrintRegexVisitor() {}
};

class PrintParseTreeVisitor: public ExprVisitor {
public:
  void Visit(StateExpr* e) { print_state(e); }
  void Visit(UnaryExpr* e);
  void Visit(BinaryExpr* e);
  static void Print(Expr *e);
private:
  std::string thema;
  void print_state(Expr *e);
  void print_arrow(Expr *src, Expr *dst);
  PrintParseTreeVisitor(): thema(" shape=circle, fillcolor=lightsteelblue1, style=filled, color = navyblue ") {}
};

class DumpExprVisitor: public ExprVisitor {
public:
  void Visit(Expr *e) {}
  void Visit(StateExpr *e);
  void Visit(UnaryExpr* e);
  void Visit(BinaryExpr* e);
  static void Dump(Expr *e);
private:
  DumpExprVisitor() {}
};

} // namespace regen

#endif /* REGEN_EXPRUTIL_H_ */
