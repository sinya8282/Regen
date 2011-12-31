#include "exprutil.h"

namespace regen {

void PrintExprVisitor::Visit(Literal* e)
{
  if (' ' <= e->literal() &&
      e->literal() <= '~') {
    switch (e->literal()) {
      case '?': case '*': case '+':
      case '.': case '|': case '\\':
      case '(': case ')': case '[': 
      case '^': case '$': case '"':
        printf("\\"); // escaping and fall through
      default:
        printf("%c", e->literal());
    }
  } else {
    printf("\\x%02x", e->literal());
  }
}

void PrintExprVisitor::Visit(CharClass* e)
{
  int c;
  bool negative = e->negative();

  printf("[");
  if (negative) {
    printf("^");
    e->set_negative(false);
  }

  for (c = 0; c < 256; c++) {
    if (e->Involve(c)) {
      if (c < 254 && e->Involve(c+1) && e->Involve(c+2)) {
        int begin = c++;
        while (++c < 255) {
          if (!e->Involve(c+1)) break;
        }
        if (' ' <= begin && begin <= '~') {
          printf("%c-", (char)begin);
        } else {
          printf("\\x%02x-", begin);
        }
      }
      if (' ' <= c && c <= '~') {
        printf("%c", c);
      } else {
        printf("\\x%02x", c);
      }
    }
  }

  printf("]");

  if (negative) e->set_negative(true);
}

void PrintExprVisitor::Print(Expr* e)
{
  static PrintExprVisitor self;
  e->Accept(&self);
}

void PrintRegexVisitor::Print(Expr* e)
{
  static PrintRegexVisitor self;
  e->Accept(&self);
  puts("");
}

void PrintRegexVisitor::Visit(BinaryExpr *e)
{
  if (Expr::SuperTypeOf(e->lhs()) == Expr::kBinaryExpr
      && e->lhs()->type() != Expr::kConcat) {
    printf("(");
    e->lhs()->Accept(this);
    printf(")");
  } else {
    e->lhs()->Accept(this);
  }

  PrintExprVisitor::Print(e);

  if (Expr::SuperTypeOf(e->rhs()) == Expr::kBinaryExpr
      && e->rhs()->type() != Expr::kConcat) {
    printf("(");
    e->rhs()->Accept(this);
    printf(")");
  } else {
    e->rhs()->Accept(this);
  }
}

void PrintRegexVisitor::Visit(UnaryExpr *e)
{
  switch (Expr::SuperTypeOf(e->lhs())) {
    case Expr::kBinaryExpr:
      printf("(");
      e->lhs()->Accept(this);
      printf(")");
      break;
    default:
      e->lhs()->Accept(this);
      break;
  }
  PrintExprVisitor::Print(e);
}

void PrintParseTreeVisitor::print_state(Expr *e)
{
  printf("  0x%p"PRIuS" [label=\"", e);
  PrintExprVisitor::Print(e);
  printf("\", %s]\n", thema.c_str());
}

void PrintParseTreeVisitor::print_arrow(Expr *src, Expr *dst)
{
  printf("  0x%p -> 0x%p\n", src, dst);
}

void PrintParseTreeVisitor::Visit(UnaryExpr* e)
{
  print_state(e);
  e->lhs()->Accept(this);
  print_arrow(e, e->lhs());
}

void PrintParseTreeVisitor::Visit(BinaryExpr* e)
{
  print_state(e);
  e->lhs()->Accept(this);
  e->rhs()->Accept(this);
  print_arrow(e, e->lhs());
  print_arrow(e, e->rhs());
}

void PrintParseTreeVisitor::Print(Expr* e)
{
  static PrintParseTreeVisitor self;
  puts("digraph ParseTree {");
  e->Accept(&self);
  puts("}");
}

void DumpExprVisitor::Visit(StateExpr *e)
{
  printf("StateExpr(%"PRIuS"): ", e->state_id());
  PrintExprVisitor::Print(e);
  Transition transition = e->transition();
  std::set<StateExpr*>::iterator iter;
  puts("");
  printf("follow:");
  iter = transition.follow.begin();
  while (iter != transition.follow.end()) {
    printf("%"PRIuS", ", (*iter)->state_id());
    ++iter;
  }
  puts("\n");
}

void DumpExprVisitor::Visit(UnaryExpr* e)
{
  e->lhs()->Accept(this);
}

void DumpExprVisitor::Visit(BinaryExpr* e)
{
  e->lhs()->Accept(this);
  e->rhs()->Accept(this);
}

void DumpExprVisitor::Dump(Expr* e)
{
  static DumpExprVisitor self;
  Transition transition = e->transition();
  std::set<StateExpr*>::iterator iter;
  printf("Start: ");
  iter = transition.first.begin();
  while (iter != transition.first.end()) {
    printf("%"PRIuS", ", (*iter)->state_id());
    ++iter;
  }
  puts("\n");

  e->Accept(&self);
}

} // namespace regen
