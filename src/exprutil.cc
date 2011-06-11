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
    printf("0x%2x", e->literal());
  }
}

void PrintExprVisitor::Visit(CharClass* e)
{
  std::size_t continuance = 256;
  int c;
  bool negative = e->negative();

  printf("[");
  if (negative) {
    printf("^");
    e->set_negative(false);
  }

  for (c = 0; c < 256; c++) {
    if (e->Involve(c)) {
      if (continuance == 256) {
        continuance = c;
      }
    } else if (continuance != 256) {
      printf("%c", (char)continuance);
      if ((std::size_t)c-1 != continuance) {
        printf("-%c", (c-1));
      }
      continuance = 256;
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
  if (e->lhs()->type() == Expr::kUnion) {
    printf("(");
    e->lhs()->Accept(this);
    printf(")");
  } else {
    e->lhs()->Accept(this);
  }

  PrintExprVisitor::Print(e);

  if (e->rhs()->type() == Expr::kUnion) {
    printf("(");
    e->rhs()->Accept(this);
    printf(")");
  } else {
    e->rhs()->Accept(this);
  }
}

void PrintRegexVisitor::Visit(UnaryExpr *e)
{
  switch (e->lhs()->type()) {
    case Expr::kConcat: case Expr::kUnion:
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
  printf("  q%"PRIuS" [label=\"", e->expr_id());
  PrintExprVisitor::Print(e);
  printf("\", %s]\n", thema.c_str());
}

void PrintParseTreeVisitor::print_arrow(Expr *src, Expr *dst)
{
  printf("  q%"PRIuS" -> q%"PRIuS"\n", src->expr_id(), dst->expr_id());
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
