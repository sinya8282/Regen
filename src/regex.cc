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

Regex::Regex(const std::string &regex, std::size_t recursive_limit):
    regex_(regex),
    recursive_limit_(recursive_limit),
    involved_char_(std::bitset<256>()),
    parse_ptr_(regex.c_str())
{
  Parse();
  MakeDFA(expr_root_, dfa_);
}

Expr::Type Regex::lex()
{
  if (*parse_ptr_ == '\0') {
    if (parse_stack_.empty()) {
      token_type_ = Expr::kEOP;
    } else {
      parse_ptr_ = parse_stack_.top();
      parse_stack_.pop();
      token_type_ = lex();
    }
  } else switch (parse_lit_ = *parse_ptr_++) {
      case '.': token_type_ = Expr::kDot;       break;
      case '[': token_type_ = Expr::kCharClass; break;
      case '|': token_type_ = Expr::kUnion;     break;
      case '?': token_type_ = Expr::kQmark;     break;
      case '+': token_type_ = Expr::kPlus;      break;
      case '*': token_type_ = Expr::kStar;      break;
      case '!': token_type_ = Expr::kNegative;  break;
      case '&': token_type_ = Expr::kIntersection;  break;
      case ')': token_type_ = Expr::kRpar;      break;
      case '^': token_type_ = Expr::kBegLine;   break;
      case '$': token_type_ = Expr::kEndLine;   break;
      case '(': {
        if (*parse_ptr_ == ')') {
          parse_ptr_++;
          token_type_ = Expr::kNone;
        } else if (*parse_ptr_     == '?' &&
            *(parse_ptr_+1) == 'R' &&
            *(parse_ptr_+2) == ')') {
          // recursive expression
          parse_ptr_ += 2;
          if (recursive_depth_ >= recursive_limit_) {
            parse_ptr_++;
            token_type_ = Expr::kNone;
          } else {
            parse_stack_.push(parse_ptr_);
            recursive_depth_++;
            parse_ptr_ = regex_.c_str();
            token_type_ = Expr::kLpar;
          }
        } else {
          token_type_ = Expr::kLpar;
        }
        break;
      }
      case '{': {
        const char *ptr = parse_ptr_;
        lower_repetition_ = 0;
        if ('0' <= *ptr && *ptr <= '9') {
          do {
            lower_repetition_ *= 10;
            lower_repetition_ += *ptr++ - '0';
          } while ('0' <= *ptr && *ptr <= '9');
        } else if (*ptr == ',') {
          lower_repetition_ = 0;
        } else {
          goto invalid;
        }
        if (*ptr == ',') {
          upper_repetition_ = 0;
          ptr++;
          if ('0' <= *ptr && *ptr <= '9') {
            do {
              upper_repetition_ *= 10;
              upper_repetition_ += *ptr++ - '0';
            } while ('0' <= *ptr && *ptr <= '9');
            if (*ptr != '}') {
              goto invalid;
            }
          } else if (*ptr == '}') {
            upper_repetition_ = -1;
          } else {
            goto invalid;
          }
        } else if (*ptr == '}') {
          upper_repetition_ = lower_repetition_;
        } else {
          goto invalid;
        }
        parse_ptr_ = ++ptr;
        if (lower_repetition_ == 0 && upper_repetition_ == -1) {
          token_type_ = Expr::kStar;
        } else if (lower_repetition_ == 1 && upper_repetition_ == -1) {
          token_type_ = Expr::kPlus;
        } else if (lower_repetition_ == 0 && upper_repetition_ == 1) {
          token_type_ = Expr::kQmark;
        } else if (lower_repetition_ == 1 && upper_repetition_ == 1) {
          token_type_ = lex();
        } else if (upper_repetition_ != -1 && upper_repetition_ < lower_repetition_) {
          exitmsg("Invalid repetition quantifier {%d,%d}",
                  lower_repetition_, upper_repetition_);
        } else {
          token_type_ = Expr::kRepetition;
        }
        break;
     invalid:
        token_type_ = Expr::kLiteral;
        break;        
      }
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

StateExpr*
Regex::CombineStateExpr(StateExpr *e1, StateExpr *e2)
{
  StateExpr *s;
  CharClass *cc = new CharClass(e1, e2);
  if (cc->count() == 256) {
    delete cc;
    s = new Dot();
  } else if (cc->count() == 1) {
    delete cc;
    char c;
    switch (e1->type()) {
      case Expr::kLiteral:
        c = ((Literal*)e1)->literal();
        break;
      case Expr::kBegLine: case Expr::kEndLine:
        c = '\n';
        break;
      default: exitmsg("Invalid Expr Type: %d", e1->type());
    }
    s = new Literal(c);
  } else {
    s = cc;
  }
  return s;
}

CharClass*
Regex::BuildCharClass() {
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

    table.set(*parse_ptr_);

    if (range) {
      for (i = (unsigned char)(*parse_ptr_) - 1; i > (unsigned char)lastc; i--) {
        table.set(i);
      }
      range = false;
    }
    lastc = *parse_ptr_;
  }
  if (*parse_ptr_ == '\0') exitmsg(" [ ] imbalance");

  if (range) {
    table.set('-');
    range = false;
  }

  parse_ptr_++;

  if (cc->count() == 1) {
    parse_lit_ = lastc;
  } else if (cc->count() >= 128 && !cc->negative()) {
    cc->set_negative(true);
    cc->flip();
  }

  return cc;
}

void Regex::Parse()
{
  Expr* e;
  StateExpr *eop;
  
  lex();

  do {
    e = e0();
  } while (e->type() == Expr::kNone);

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
  e = new Concat(e, eop);
  e->set_expr_id(++expr_id_);

  expr_root_ = e;
  expr_root_->FillTransition();
}

/* Regen parsing rules
 * RE ::= e0 EOP
 * e0 ::= e1 ('|' e1)*                    # union
 * e1 ::= e2 ('&' e2)*                    # intersection
 * e2 ::= e3+                             # concatenation
 * e3 ::= e4 ([?+*]|{N,N}|{,}|{,N}|{N,})* # repetition
 * e4 ::= ATOM | '(' e0 ')' | '!' e0      # ATOM, grouped expresion, negative expresion
*/

Expr *Regex::e0()
{
  Expr *e, *f;
  e = e1();
  
  while (e->type() == Expr::kNone &&
         token_type_ == Expr::kUnion) {
    lex();
    e = e1();
  }
  
  while (token_type_ == Expr::kUnion) {
    lex();
    f = e1();
    if (f->type() != Expr::kNone) {
      if (e->stype() == Expr::kStateExpr &&
          f->stype() == Expr::kStateExpr) {
        e = CombineStateExpr((StateExpr*)e, (StateExpr*)f);
      } else {
        e = new Union(e, f);
      }
    }
  }
  return e;
}

Expr *
Regex::e1()
{
  Expr *e;
  std::vector<Expr*> exprs;

  e = e2();

  while (e->type() == Expr::kNone &&
         token_type_ == Expr::kIntersection) {
    lex();
    e = e2();
  }

  exprs.push_back(e);

  while (token_type_ == Expr::kIntersection) {
    lex();
    e = e2();
    if (e->type() != Expr::kNone) {
      exprs.push_back(e);
    }
  }

  if (exprs.size() == 1) {
    e = exprs[0];
  } else {
    std::vector<Expr*>::iterator iter = exprs.begin();
    e = new Concat(*iter, new EOP());
    *iter = e;
    ++iter;
    while (iter != exprs.end()) {
      *iter = new Concat(*iter, new EOP());
      e = new Union(e, *iter);
      ++iter;
    }
    
    DFA dfa;
    e->FillTransition();
    MakeDFA(e, dfa, exprs.size());
    e = CreateRegexFromDFA(dfa);

    iter = exprs.begin();
    while (iter != exprs.end()) {
      delete *iter;
      ++iter;
    }
  }
  
  return e;
}

Expr *
Regex::e2()
{
  Expr *e, *f;
  e = e3();
  
  while (e->type() == Expr::kNone &&
         (token_type_ == Expr::kLiteral ||
          token_type_ == Expr::kCharClass ||
          token_type_ == Expr::kDot ||
          token_type_ == Expr::kEndLine ||
          token_type_ == Expr::kBegLine ||
          token_type_ == Expr::kNone ||
          token_type_ == Expr::kLpar ||
          token_type_ == Expr::kNegative)) {
    e = e3();
  }

  while (token_type_ == Expr::kLiteral ||
         token_type_ == Expr::kCharClass ||
         token_type_ == Expr::kDot ||
         token_type_ == Expr::kEndLine ||
         token_type_ == Expr::kBegLine ||
         token_type_ == Expr::kNone ||
         token_type_ == Expr::kLpar ||
         token_type_ == Expr::kNegative) {
    f = e3();
    if (f->type() != Expr::kNone) {
      e = new Concat(e, f);
    }
  }

  return e;
}

Expr *
Regex::e3()
{
  Expr *e;
  e = e4();
  bool infinity = false, nullable = false;

looptop:
  switch (token_type_) {
    case Expr::kStar:
      infinity = true;
      nullable = true;
      goto loop;
    case Expr::kPlus:
      infinity = true;
      goto loop;
    case Expr::kQmark:
      nullable = true;
      goto loop;
    default: goto loopend;
  }
loop:
  lex();
  goto looptop;
loopend:

  if (e->type() != Expr::kNone &&
      (infinity || nullable)) {
    if (infinity && nullable) {
      e = new Star(e);
    } else if (infinity) {
      e = new Plus(e);
    } else { //nullable
      e = new Qmark(e);
    }
  }
  
  if (token_type_ == Expr::kRepetition) {
    if (e->type() == Expr::kNone) goto loop;
    if (lower_repetition_ == 0 && upper_repetition_ == 0) {
      delete e;
      e = new None();
    } else if (upper_repetition_ == -1) {
      Expr* f = e;
      for (int i = 0; i < lower_repetition_ - 2; i++) {
        e = new Concat(e, f->Clone());
      }
      e = new Concat(e, new Plus(f->Clone()));
    } else if (upper_repetition_ == lower_repetition_) {
      Expr* f = e;
      for (int i = 0; i < lower_repetition_ - 1; i++) {
        e = new Concat(e, f->Clone());
      }
    } else {
      Expr *f = e;
      for (int i = 0; i < lower_repetition_ - 1; i++) {
        e = new Concat(e, f->Clone());
      }
      if (lower_repetition_ == 0) {
        e = new Qmark(e);
        lower_repetition_ = 1;
      }
      for (int i = 0; i < (upper_repetition_ - lower_repetition_); i++) {
        e = new Concat(e, new Qmark(f->Clone()));
      }
    }
    infinity = false, nullable = false;
    goto loop;
  }

  return e;
}

Expr *
Regex::e4()
{
  Expr *e;

  switch(token_type_) {
    case Expr::kLiteral:
      e = new Literal(parse_lit_);
      break;
    case Expr::kBegLine:
      e = new BegLine();
      break;
    case Expr::kEndLine:
      e = new EndLine();
      break;
    case Expr::kDot:
      e = new Dot();
      break;
    case Expr::kCharClass: {
      CharClass *cc = BuildCharClass();
      if (cc->count() == 1) {
        e = new Literal(parse_lit_);
        delete cc;
      } else if (cc->count() == 256) {
        e = new Dot();
        delete cc;
      } else {
        e = cc;
      }
      break;
    }
    case Expr::kNone:
      e = new None();
      break;
    case Expr::kLpar:
      lex();
      e = e0();
      if (token_type_ != Expr::kRpar) exitmsg("expected a ')'");
      break;
    case Expr::kNegative: {
      bool negative = false;
      do {
        negative = !negative;
        lex();
      } while (token_type_ == Expr::kNegative);
      e = e4();
      if (negative) {
        DFA dfa;
        e = new Concat(e, new EOP());
        e->FillTransition();
        Expr *e_ = e;
        MakeDFA(e, dfa);
        dfa.Negative();
        e = CreateRegexFromDFA(dfa);
        delete e_;
      }
      return e;
    }
    case Expr::kRpar:
      exitmsg("expected a '('!");
    case Expr::kEOP:
      exitmsg("expected none-nullable expression.");
    default:
      exitmsg("can't handle Expr: %s", Expr::TypeString(token_type_));
  }

  lex();

  return e;
}

void Regex::MakeDFA(Expr* expr_root, DFA &dfa, std::size_t neop)
{
  std::size_t dfa_id = 0;

  typedef std::set<StateExpr*> NFA;
  
  std::map<NFA, int> dfa_map;
  std::queue<NFA> queue;
  NFA default_next;
  NFA first_states = expr_root->transition().first;

  dfa_map[default_next] = DFA::REJECT;
  dfa_map[first_states] = dfa_id++;
  queue.push(first_states);

  while (!queue.empty()) {
    NFA nfa_states = queue.front();
    queue.pop();
    std::vector<NFA> transition(256);
    NFA::iterator iter = nfa_states.begin();
    bool is_accept = false;
    default_next.clear();
    std::set<EOP*> eops;

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
          eops.insert((EOP*)(*iter));
          if (eops.size() == neop) {
            is_accept = true;
          }
          break;
        }
        case Expr::kNone: break;
        default: exitmsg("notype");
      }
      ++iter;
    }

    /*
    std::vector<NFA>::iterator trans = transition.begin();
    while (trans != transition.end()) {
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
    */
    
    DFA::Transition &dfa_transition = dfa.get_new_transition();
    std::set<int> dst_state;
    // only support Most-Left-Shortest matching
    //if (is_accept) goto settransition;
    
    for (int i = 0; i < 256; i++) {
      NFA &next = transition[i];
      if (next.empty()) continue;

      if (dfa_map.find(next) == dfa_map.end()) {
        dfa_map[next] = dfa_id++;
        queue.push(next);
      }
      dfa_transition[i] = dfa_map[next];
      dst_state.insert(dfa_map[next]);
    }
    //settransition:
    dfa.set_state_info(is_accept, dfa_map[default_next], dst_state);
  }
}

// Converte DFA to Regular Expression using GNFA.
Expr* Regex::CreateRegexFromDFA(DFA &dfa)
{
  int GSTART  = dfa.size();
  int GACCEPT = GSTART+1;

  typedef std::map<int, Expr*> GNFATrans;
  std::vector<GNFATrans> gnfa_transition(GACCEPT);

  for (std::size_t i = 0; i < dfa.size(); i++) {
    const DFA::Transition &transition = dfa.GetTransition(i);
    GNFATrans &gtransition = gnfa_transition[i];
    for (int c = 0; c < 256; c++) {
      int next = transition[c];
      if (next != DFA::REJECT) {
        Expr *e;
        if (c < 255 && next == transition[c+1]) {
          int begin = c;
          while (++c < 255) {
            if (transition[c] != transition[c+1]) break;
          }
          int end = c;
          if (begin == 0 && end == 255) {
            e = new Dot();
          } else {
            std::bitset<256> table;
            bool negative = false;
            for (int j = begin; j <= end; j++) {
              table.set(j);
            }
            if (table.count() >= 128) {
              negative = true;
              table.flip();
            }
            e = new CharClass(table, negative);
          }
        } else {
          e = new Literal(c);
        }
        if (gtransition.find(next) != gtransition.end()) {
          Expr* f = gtransition[next];
          if (e->stype() == Expr::kStateExpr &&
              f->stype() == Expr::kStateExpr) {
            e = CombineStateExpr((StateExpr*)e, (StateExpr*)f);
          } else {
            e = new Union(e, f);
          }
        }
        gtransition[next] = e;
      }
    }
  }

  for (std::size_t i = 0; i < dfa.size(); i++) {
    if (dfa.IsAcceptState(i)) {
      gnfa_transition[i][GACCEPT] = NULL;
    }
  }

  gnfa_transition[GSTART][0] = NULL;
  
  for (int i = 0; i < GSTART; i++) {
    Expr* loop = NULL;
    GNFATrans &gtransition = gnfa_transition[i];
    if (gtransition.find(i) != gtransition.end()) {
      loop = new Star(gtransition[i]);
      gtransition.erase(i);
    }
    for (int j = i+1; j <= GSTART; j++) {
      if (gnfa_transition[j].find(i) != gnfa_transition[j].end()) {
        Expr* regex1 = gnfa_transition[j][i];
        gnfa_transition[j].erase(i);
        GNFATrans::iterator iter = gtransition.begin();
        while (iter != gtransition.end()) {
          Expr* regex2 = (*iter).second;
          if (loop != NULL) {
            if (regex2 != NULL) {
              regex2 = new Concat(loop, regex2);
            } else {
              regex2 = loop;
            }
          }
          if (regex1 != NULL) {
            if (regex2 != NULL) {
              regex2 = new Concat(regex1, regex2);
            } else {
              regex2 = regex1;
            }
          }
          if (regex2 != NULL) {
            regex2 = regex2->Clone();
          }
          if (gnfa_transition[j].find((*iter).first) != gnfa_transition[j].end()) {
            if (gnfa_transition[j][(*iter).first] != NULL) {
              if (regex2 != NULL) {
                Expr* e = gnfa_transition[j][(*iter).first];
                Expr* f = regex2;
                if (e->stype() == Expr::kStateExpr &&
                    f->stype() == Expr::kStateExpr) {
                  e = CombineStateExpr((StateExpr*)e, (StateExpr*)f);
                } else {
                  e = new Union(e, f);
                }
                gnfa_transition[j][(*iter).first] = e;
              } else {
                gnfa_transition[j][(*iter).first] =
                    new Qmark(gnfa_transition[j][(*iter).first]);
              }
            } else {
              if (regex2 != NULL) {
                gnfa_transition[j][(*iter).first] = new Qmark(regex2);
              } else {
                gnfa_transition[j][(*iter).first] = regex2;
              }
            }
          } else {
            gnfa_transition[j][(*iter).first] = regex2;
          }
          ++iter;
        }
      }
    }
    GNFATrans::iterator iter = gtransition.begin();
    iter = gtransition.begin();
    while (iter != gtransition.end()) {
      if ((*iter).second != NULL) {
        delete (*iter).second;
      }
      ++iter;
    }
    if (loop != NULL) {
      delete loop;
    }
  }

  if(gnfa_transition[GSTART][GACCEPT] == NULL) {
    return new None();
  } else {
    return gnfa_transition[GSTART][GACCEPT];
  }
}

bool Regex::FullMatch(const std::string &string)  const {
  const unsigned char* begin = (const unsigned char *)string.c_str();
  return FullMatch(begin, begin+string.length());
}


bool Regex::FullMatch(const unsigned char *begin, const unsigned char * end)  const {
  return dfa_.FullMatch(begin, end);
}

void Regex::PrintRegex() {
  PrintRegexVisitor::Print(expr_root_);
}

void Regex::PrintParseTree() const {
  PrintParseTreeVisitor::Print(expr_root_);
}

void Regex::DumpExprTree() const {
  DumpExprVisitor::Dump(expr_root_);
}

} // namespace regen
 
