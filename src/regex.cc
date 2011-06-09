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

Regex::Regex(const std::string &regex, std::size_t recursive_depth = 2):
    regex_(regex),
    recursive_depth_(recursive_depth),
    involved_char_(std::bitset<256>()),
    parse_ptr_(regex.c_str()),
    dfa_(DFA())
{
  Parse();
  CreateDFA();
}

Expr::Type Regex::lex()
{
  if (*parse_ptr_ == '\0') {
    if (recursive_stack_.empty()) {
      token_type_ = Expr::kEOP;
    } else {
      parse_ptr_ = recursive_stack_.top();
      recursive_stack_.pop();
      token_type_ = Expr::kRpar;      
    }
  } else switch (parse_lit_ = *parse_ptr_++) {
      case '.': token_type_ = Expr::kDot;       break;
      case '[': token_type_ = Expr::kCharClass; break;
      case '|': token_type_ = Expr::kUnion;     break;
      case '?': token_type_ = Expr::kQmark;     break;
      case '+': token_type_ = Expr::kPlus;      break;
      case '*': token_type_ = Expr::kStar;      break;
      case '(': {
        if (*parse_ptr_     == '?' &&
            *(parse_ptr_+1) == 'R' &&
            *(parse_ptr_+2) == ')') {
          parse_ptr_ += 3;
          if (recursive_stack_.size() >= recursive_depth_) {
            return lex();
          } else {
            recursive_stack_.push(parse_ptr_);
            parse_ptr_ = regex_.c_str();
            token_type_ = Expr::kLpar;
          }
        } else {
          token_type_ = Expr::kLpar;
        }
        break;
      }
      case ')': token_type_ = Expr::kRpar;      break;
      case '^': token_type_ = Expr::kBegLine;   break;
      case '$': token_type_ = Expr::kEndLine;   break;
      case '\\':
        token_type_ = Expr::kLiteral;
        if (*parse_ptr_ == '\0') exitmsg("bad '\\'");
        parse_lit_ = *parse_ptr_++;
        break;
      default:
        token_type_ = Expr::kLiteral;
  }
  return token_type_;
}

CharClass* Regex::BuildCharClass() {
  std::size_t i;
  CharClass *cc = new CharClass();
  std::bitset<256>& table = cc->table();
  bool range;
  char lastc;

  if (*parse_ptr_ == '^') {
    parse_ptr_++;
    cc->set_negative(true);
  }

  if (*parse_ptr_ == ']' || *parse_ptr_ == '-') {
    table.set((unsigned char)*parse_ptr_);
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

    table.set(*parse_ptr_, true);

    if (range) {
      for (i = (unsigned char)(*parse_ptr_) - 1; i > (unsigned char)lastc; i--) {
        table.set(i, true);
      }
      range = false;
    }
    lastc = *parse_ptr_;
  }
  if (*parse_ptr_ == '\0') exitmsg(" [ ] imbalance");

  if (range) {
    table.set('-', true);
    range = false;
  }

  parse_ptr_++;

  if (cc->count() == 1) {
    parse_lit_ = lastc;
  }

  return cc;
}

void Regex::Parse()
{
  Expr* e;
  StateExpr *eop;
  
  lex();

  e = e0();

  if (token_type_ != Expr::kEOP) exitmsg("expected end of pattern.");

  // add '.*' to top of regular expression.
  // Expr *dotstar;
  // StateExpr *dot;
  // dot = new Dot();
  // dot->set_expr_id(++expr_id_);
  // dot->set_state_id(++state_id_);
  // dotstar = new Star(dot);
  // dotstar->set_expr_id(++expr_id_);
  // e = new Concat(dotstar, e);
  // e->set_expr_id(++expr_id_);
  
  eop = new EOP();
  eop->set_expr_id(0);
  eop->set_state_id(0);
  states_.push_front(eop);
  e = new Concat(e, eop);
  e->set_expr_id(++expr_id_);

  expr_root_ = e;
  expr_root_->FillTransition();
}

Expr *Regex::e0()
{
  Expr *e, *f;
  e = e1();
  
  while (token_type_ == Expr::kUnion) {
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

  while (token_type_ == Expr::kLiteral ||
     token_type_ == Expr::kCharClass ||
     token_type_ == Expr::kDot ||
     token_type_ == Expr::kLpar ||
     token_type_ == Expr::kEndLine ||
     token_type_ == Expr::kBegLine) {
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

  while (token_type_ == Expr::kStar ||
     token_type_ == Expr::kPlus ||
     token_type_ == Expr::kQmark) {
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
    case Expr::kLiteral:
      s = new Literal(parse_lit_);
      goto setid;
    case Expr::kBegLine:
      s = new BegLine();
      goto setid;
    case Expr::kEndLine:
      s = new EndLine();
      goto setid;
    case Expr::kDot:
      s = new Dot();
      goto setid;
    case Expr::kCharClass: {
      CharClass *cc = BuildCharClass();
      if (cc->count() == 1) {
        // '[a]' just be matched 'a'.
        s = new Literal(parse_lit_);
        delete cc;
      } else {
        s = cc;
      }
      goto setid;
    }
    case Expr::kLpar:
      lex();
      e = e0();
      if (token_type_ != Expr::kRpar) exitmsg("expected a ')'");
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
  NFA first_states = expr_root_->transition().first;

  dfa_map[default_next] = DFA::REJECT;
  dfa_map[first_states] = dfa_id++;
  queue.push(first_states);

  while(!queue.empty()) {
    NFA nfa_states = queue.front();
    queue.pop();
    std::vector<NFA> transition(256);
    NFA::iterator iter = nfa_states.begin();
    bool is_accept = false;
    default_next.clear();

    while (iter != nfa_states.end()) {
      NFA &next = (*iter)->transition().follow;
      switch ((*iter)->type()) {
        case Expr::kLiteral: {
          Literal* literal = static_cast<Literal*>(*iter);
          unsigned char index = (unsigned char)literal->literal();
          transition[index].insert(next.begin(), next.end());
          break;
        }
        case Expr::kCharClass: {
          CharClass* charclass = static_cast<CharClass*>(*iter);
          for (int i = 0; i < 256; i++) {
            if (charclass->Involve(static_cast<unsigned char>(i))) {
              transition[i].insert(next.begin(), next.end());
            }
          }
          break;
        }
        case Expr::kDot: {
          for (int i = 0; i < 256; i++) {
            transition[i].insert(next.begin(), next.end());
          }
          default_next.insert(next.begin(), next.end());
          break;
        }
        case Expr::kBegLine: {
          transition['\n'].insert(next.begin(), next.end());
          break;
        }
        case Expr::kEndLine: {
          NFA::iterator next_iter = next.begin();
          while (next_iter != next.end()) {
            transition['\n'].insert(*next_iter);
            ++next_iter;
          }
          break;
        }
        case Expr::kEOP: {
          is_accept = true;
          break;
        }
        default: exitmsg("notype");
      }
      ++iter;
    }

    std::vector<NFA>::iterator trans = transition.begin();
    while (trans != transition.end() && false) {
      std::set<StateExpr*>::iterator next_ = trans->begin();
      while (next_ != trans->end()) {
        if ((*next_)->type() == Expr::kEOP) goto loopend;
        ++next_;
      }
      trans->insert(first_states.begin(), first_states.end());
   loopend:
      ++trans;
    }
    default_next.insert(first_states.begin(), first_states.end());
    
    DFA::Transition &dfa_transition = dfa_.get_new_transition();
    std::set<int> dst_state;
    //if (is_accept) goto settransition; // only support Most-Left-Shortest matching
    
    for (int i = 0; i < 256; i++) {
      const NFA &next = transition[i];
      if (next.empty()) continue;

      if (dfa_map.find(next) == dfa_map.end()) {
        dfa_map[next] = dfa_id++;
        queue.push(next);
      }
      dfa_transition[i] = dfa_map[next];
      dst_state.insert(dfa_map[next]);
    }
    //settransition:
    dfa_.set_state_info(is_accept, dfa_map[default_next], dst_state);
  }
}

bool Regex::FullMatch(const std::string &string)  const {
  const unsigned char* begin = (const unsigned char *)string.c_str();
  return FullMatch(begin, begin+string.length());
}


bool Regex::FullMatch(const unsigned char *begin, const unsigned char * end)  const {
  return dfa_.FullMatch(begin, end);
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
