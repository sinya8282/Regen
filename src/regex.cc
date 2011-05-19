#include "regex.h"
#include "exprutil.h"

// for debug
void nfadump(const std::set<regen::StateExpr*> &nfa, bool verbose = false)
{
  std::set<regen::StateExpr*>::iterator iter = nfa.begin();
  while (iter != nfa.end()) {
    printf("%"PRIuS"", (*iter)->state_id());
    if (verbose) {
      printf("(");
      regen::PrintExprVisitor::Print(*iter);
      printf(")");
    }
    printf(", ");
    ++iter;
  }
  puts("");
}

namespace regen {

Regex::Regex(const std::string &regex):
    regex_(regex),
    parse_ptr_(regex.c_str()),
    involved_char_(std::vector<bool>(256, false)),
    dfa_(DFA())
{
  Parse();
  CreateDFA();
}

Expr::Type Regex::lex()
{
  if (*parse_ptr_ == '\0') {
    return (token_type_ = Expr::EOP);
  } else switch (parse_lit_ = *parse_ptr_++) {
      case '.': token_type_ = Expr::Dot;       break;
      case '[': token_type_ = Expr::CharClass; break;
      case '|': token_type_ = Expr::Union;     break;
      case '?': token_type_ = Expr::Qmark;     break;
      case '+': token_type_ = Expr::Plus;      break;
      case '*': token_type_ = Expr::Star;      break;
      case '(': token_type_ = Expr::Lpar;      break;
      case ')': token_type_ = Expr::Rpar;      break;
      case '^': token_type_ = Expr::BegLine;   break;
      case '$': token_type_ = Expr::EndLine;   break;
      case '\\':
        token_type_ = Expr::Literal;
        if (*parse_ptr_ == '\0') exitmsg("bad '\\'");
        parse_lit_ = *parse_ptr_++;
        break;
      default:
        token_type_ = Expr::Literal;
  }
  return token_type_;
}

std::size_t Regex::MakeCharClassTable(std::vector<bool> &table) {
  std::size_t i;
  std::size_t count = 0;
  bool comp, range;
  char lastc;

  if (*parse_ptr_ == '^') {
    parse_ptr_++;
    for (i = ' '; i <= '~'; i ++) {
      // ascii charactor code only
      table[i] = true;
    }
    comp = false;
  } else {
    comp = true;
  }

  if (*parse_ptr_ == ']' || *parse_ptr_ == '-') {
    table[(unsigned char)*parse_ptr_] = true;
    lastc = *parse_ptr_++;
  }

  for (range = false; (*parse_ptr_ != '\0') && (*parse_ptr_ != ']'); parse_ptr_++) {
    if (!range && *parse_ptr_ == '-') {
      range = true;
      continue;
    }

    if (*parse_ptr_ == '\\') {
      if (*(parse_ptr_++) == '\0') {
        exitmsg(" [ ] imbalance");
      }
    }

    table[(unsigned char)*parse_ptr_] = comp;

    if (range) {
      for (i = (unsigned char)(*parse_ptr_) - 1; i > (unsigned char)lastc; i--) {
        table[i] = comp;
      }
      range = false;
    }
    lastc = *parse_ptr_;
  }
  if (*parse_ptr_ == '\0') exitmsg(" [ ] imbalance");

  if (range) {
    table['-'] = comp;
    range = false;
  }

  parse_ptr_++;

  for (i = 0; i < 256; i++) {
    if (table[i]) {
      lastc = (char)i;
      count++;
    }
  }

  if (count == 1) {
    parse_lit_ = lastc;
  }

  return count;
}

void Regex::Parse()
{
  Expr* e;
  StateExpr *eop;
  
  lex();

  e = e0();

  if (token_type_ != Expr::EOP) exitmsg("expected end of pattern.");

  // add '.*' to top of regular expression.
  Expr *dotstar;
  StateExpr *dot;
  dot = new Dot();
  dot->set_expr_id(++expr_id_);
  dot->set_state_id(++state_id_);
  dotstar = new Star(dot);
  dotstar->set_expr_id(++expr_id_);
  e = new Concat(dotstar, e);
  e->set_expr_id(++expr_id_);
  
  eop = new EOP();
  eop->set_expr_id(0);
  eop->set_state_id(0);
  e = new Concat(e, eop);
  e->set_expr_id(++expr_id_);

  expr_root_ = e;
  expr_root_->FillTransition();
}

Expr *Regex::e0()
{
  Expr *e, *f;
  e = e1();
  
  while (token_type_ == Expr::Union) {
  lex();
    f = e1();
    e = new Union(e, f);
    e->set_expr_id(++expr_id_);
  }
  return e;
}

Expr *
Regex::e1()
{
  Expr *e, *f;
  e = e2();

  while (token_type_ == Expr::Literal ||
     token_type_ == Expr::CharClass ||
     token_type_ == Expr::Dot ||
     token_type_ == Expr::Lpar ||
     token_type_ == Expr::EndLine ||
     token_type_ == Expr::BegLine) {
  f = e2();
  e = new Concat(e, f);
  e->set_expr_id(++expr_id_);
  }

  return e;
}

Expr *
Regex::e2()
{
  Expr *e;
  e = e3();

  while (token_type_ == Expr::Star ||
     token_type_ == Expr::Plus ||
     token_type_ == Expr::Qmark) {
    e = UnaryExpr::DispatchNew(token_type_, e);
    e->set_expr_id(++expr_id_);
    lex();
  }

  return e;
}

Expr *
Regex::e3()
{
  Expr *e;
  StateExpr *s;

  switch(token_type_) {
    case Expr::Literal:
      s = new Literal(parse_lit_);
      goto setid;
    case Expr::BegLine:
      s = new BegLine();
      goto setid;
    case Expr::EndLine:
      s = new EndLine();
      goto setid;
    case Expr::Dot:
      s = new Dot();
      goto setid;
    case Expr::CharClass: {
      CharClass *cc = new CharClass();
      std::vector<bool> &table = cc->table();
      cc->set_count(MakeCharClassTable(table));
      if (cc->count() == 1) {
        // '[a]' just be matched 'a'.
        s = new Literal(parse_lit_);
        delete cc;
      } else {
        s = cc;
      }
      goto setid;
    }
    case Expr::Lpar:
      lex();
      e = e0();
      if (token_type_ != Expr::Rpar) exitmsg("expected a ')'");
      goto noid;
    default:
      exitmsg("expected a Literal or '('!");
  }

setid:
  s->set_expr_id(++expr_id_);
  s->set_state_id(++state_id_);
  states_.push_back(s);
  e = s;
noid:
  lex();

  return e;
}

void Regex::CreateDFA()
{
  std::size_t dfa_id = 0;

  typedef std::set<StateExpr*> NFA;
  
  std::map<NFA, int> dfa_map;
  std::queue<NFA> queue;
  NFA default_next;
  NFA nfa_states = expr_root_->transition().first;

  dfa_map.insert(std::map<NFA ,int>::value_type(default_next, DFA::REJECT));
  dfa_map.insert(std::map<NFA ,int>::value_type(nfa_states, dfa_id++));
  queue.push(nfa_states);

  while(!queue.empty()) {
    nfa_states = queue.front();
    queue.pop();
    std::vector<NFA> transition(256);
    NFA::iterator iter = nfa_states.begin();
    bool is_accept = false;
    default_next.clear();

    while (iter != nfa_states.end()) {
      NFA &next = (*iter)->transition().follow;
      switch ((*iter)->type()) {
        case Expr::Literal: {
          Literal* literal = static_cast<Literal*>(*iter);
          unsigned char index = (unsigned char)literal->literal();
          transition[index].insert(next.begin(), next.end());
          break;
        }
        case Expr::CharClass: {
          CharClass* charclass = static_cast<CharClass*>(*iter);
          for (int i = 0; i < 256; i++) {
            if (charclass->Involve(static_cast<unsigned char>(i))) {
              transition[i].insert(next.begin(), next.end());
            }
          }
          break;
        }
        case Expr::Dot: {
          for (int i = 0; i < 256; i++) {
            transition[i].insert(next.begin(), next.end());
          }
          default_next.insert(next.begin(), next.end());
          break;
        }
        case Expr::BegLine: {
          transition['\n'].insert(next.begin(), next.end());
          break;
        }
        case Expr::EndLine: {
          NFA::iterator next_iter = next.begin();
          while (next_iter != next.end()) {
            transition['\n'].insert(*next_iter);
            ++next_iter;
          }
          break;
        }
        case Expr::EOP: {
          is_accept = true;
          break;
        }
        default: exitmsg("notype");
      }
      ++iter;
    }

    std::vector<int> dfa_transition(256, DFA::REJECT);
    if (is_accept) goto settransition; // only support Most-Left-Shortest matching
    
    for (int i = 0; i < 256; i++) {
      NFA &next = transition[i];
      if (next.empty()) continue;

      if (dfa_map.find(next) == dfa_map.end()) {
        dfa_map.insert(std::map<NFA, std::size_t>::value_type(next, dfa_id++));
        queue.push(next);
      }
      dfa_transition[i] = dfa_map[next];
    }
 settransition:
    dfa_.set_transition(dfa_transition, is_accept, dfa_map[default_next]);
  }
}

void Regex::PrintRegex() const {
  PrintRegexVisitor::Print(expr_root_);
}

void Regex::PrintParseTree() const {
  PrintParseTreeVisitor::Print(expr_root_);
}

void Regex::DumpExprTree() const {
  DumpExprVisitor::Dump(expr_root_);
}

} // namespace regen
