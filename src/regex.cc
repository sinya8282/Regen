#include "regex.h"

namespace regen {

Regex::Regex(const std::string &regex, const Regen::Options flags):
    regex_(regex),
    flag_(flags),
    recursive_depth_(0),
    xor_num_(0),
    involved_char_(std::bitset<256>()),
    olevel_(Regen::Options::Onone),
    dfa_failure_(false),
    dfa_(flags)
{
  const unsigned char *begin = (const unsigned char*)regex.c_str(),
      *end = begin + regex.length();
  Lexer lexer(begin, end, flags);
  expr_root_ = Parse(&lexer);
  NumberingStateExprVisitor::Numbering(expr_root_, &state_exprs_);
  dfa_.set_expr_root(expr_root_);
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

Expr* Regex::Parse(Lexer *lexer)
{
  Expr* e;

  lexer->Consume();

  do {
    e = e0(lexer);
  } while (e->type() == Expr::kNone);

  if (lexer->token() != Lexer::kEOP) exitmsg("expected end of pattern.");

  if (flag_.partial_match()) {
    //add '.*?' to top of regular expression for Partial Match
    Expr* dotstar = new Star(new Dot(), true);
    e = new Concat(dotstar, e);
  }

  StateExpr *eop = new EOP();
  if (flag_.reverse_match()) { 
    e = new Concat(eop, e);
    e->FillPosition();
    e->FillReverseTransition();
  } else {
    e = new Concat(e, eop);
    e->FillPosition();
    e->FillTransition();
  }

  return e;
}

/* Regen parsing rules
 * RE ::= e0 EOP
 * e0 ::= e1 ('|' e1)*                    # union
 * e1 ::= e2 ('&' e2)*                    # intersection
 * e2 ::= e3+                             # concatenation
 * e3 ::= e4 ([?+*]|{N,N}|{,}|{,N}|{N,})* # repetition
 * e4 ::= ATOM | '(' e0 ')' | '!' e0      # ATOM, grouped expresion, complement expresion
*/

Expr* Regex::e0(Lexer *lexer)
{
  Expr *e, *f;
  e = e1(lexer);
  
  while (e->type() == Expr::kNone &&
         lexer->token() == Lexer::kUnion) {
    lexer->Consume();
    e = e1(lexer);
  }
  
  while (lexer->token() == Lexer::kUnion) {
    lexer->Consume();
    f = e1(lexer);
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
Regex::e1(Lexer *lexer)
{
  Expr *e, *f;
  e = e2(lexer);
  bool null = e->type() == Expr::kNone;

  while (lexer->token() == Lexer::kIntersection) {
    lexer->Consume();
    if (!null) {
      f = e2(lexer);
      if (f->type() == Expr::kNone) {
        delete e;
        e = f;
        null = true;
      } else {
        e = new Intersection(e, f);
      }
    }
  }
  
  return e;
}

Expr *
Regex::e2(Lexer *lexer)
{
  Expr *e, *f;
  e = e3(lexer);
  
  while (e->type() == Expr::kNone &&
         lexer->Concatenated()) {
    e = e3(lexer);
  }

  while (lexer->Concatenated()) {
    f = e3(lexer);
    if (f->type() != Expr::kNone) {
      e = new Concat(e, f);
    }
  }

  return e;
}

Expr *
Regex::e3(Lexer *lexer)
{
  Expr *e;
  e = e4(lexer);

  if (e->type() == Expr::kNone) {
    while (lexer->Quantifier()) {
      lexer->Consume();
    }
    return e;
  }
  
  while (lexer->Quantifier()) {
    bool non_greedy = false;
    Lexer::Type token = lexer->token();
    lexer->Consume();
    if (lexer->token() == Lexer::kQmark) {
      non_greedy = true;
      lexer->Consume();
    }
    switch (token) {
      case Lexer::kStar:
        e = new Star(e, non_greedy);
        break;
      case Lexer::kPlus: {
        if (non_greedy) {
          Expr* e_ = e->Clone();
          e = new Concat(e_, new Star(e, non_greedy));
        } else {
          e = new Plus(e);
        }
        break;
      }
      case Lexer::kQmark: {
        e = new Qmark(e, non_greedy);
        break;
      }
      case Lexer::kRepetition: {
        std::pair<int, int> r = lexer->repetition();
        int lower_repetition = r.first, upper_repetition = r.second;
        if (lower_repetition == 0 && upper_repetition == 0) {
          delete e;
          e = new None();
        } else if (upper_repetition == -1) {
          Expr* f = e;
          for (int i = 0; i < lower_repetition - 1; i++) {
            e = new Concat(e, f->Clone());
          }
          e = new Concat(e, new Star(f->Clone(), non_greedy));
        } else if (upper_repetition == lower_repetition) {
          Expr* f = e;
          for (int i = 0; i < lower_repetition - 1; i++) {
            e = new Concat(e, f->Clone());
          }
        } else {
          Expr *f = e;
          for (int i = 0; i < lower_repetition - 1; i++) {
            e = new Concat(e, f->Clone());
          }
          if (lower_repetition == 0) {
            e = new Qmark(e, non_greedy);
            lower_repetition = 1;
          }
          for (int i = 0; i < (upper_repetition - lower_repetition); i++) {
            e = new Concat(e, new Qmark(f->Clone(), non_greedy));
          }
        }
        break;
      }
      default:
        break;        
    }
  }
  
  return e;
}

Expr *
Regex::e4(Lexer *lexer)
{ 
  Expr *e;

  switch(lexer->token()) {
    case Lexer::kLiteral:
      e = new Literal(lexer->literal());
      break;
    case Lexer::kBegLine:
      e = new BegLine();
      break;
    case Lexer::kEndLine:
      e = new EndLine();
      break;
    case Lexer::kDot:
      e = new Dot();
      break;
    case Lexer::kByteRange: {
      e = new CharClass(lexer->table());
      break;
    }
    case Lexer::kCharClass: {
      CharClass *cc = BuildCharClass(lexer);
      if (cc->count() == 1) {
        e = new Literal(lexer->literal());
        delete cc;
      } else if (cc->count() == 256) {
        e = new Dot();
        delete cc;
      } else {
        e = cc;
      }
      break;
    }
    case Lexer::kNone:
      e = new None();
      break;
    case Lexer::kBackRef:
      if (lexer->backref() > lexer->groups().size()) exitmsg("Invalid back reference");
      e = lexer->groups()[lexer->backref()-1]->Clone();
      break;
    case Lexer::kLpar:
      lexer->Consume();
      e = e0(lexer);
      if (lexer->token() != Lexer::kRpar) exitmsg("expected a ')'");
      lexer->groups().push_back(e);
      break;
    case Lexer::kComplement: {
      bool complement = false;
      do {
        complement = !complement;
        lexer->Consume();
      } while (lexer->token() == Lexer::kComplement);
      e = e4(lexer);
      if (complement) {
        if (e->type() == Expr::kNone) {
          delete e;
          e = new Star(new Dot());
          return e;
        }
        XOR *e_ = new XOR(new Star(new Dot), e);
        e_->set_id(xor_num_++);
        e = e_;
      }
      return e;
    }
    case Lexer::kRecursive: {
      lexer->Consume();
      std::pair<int, int> recursive_limit;
      recursive_limit.first = recursive_limit.second = 1;
      if (lexer->token() == Lexer::kRepetition) {
        recursive_limit = lexer->repetition();
        lexer->Consume();
      }
      if (recursive_limit.second == -1) {
        exitmsg("disallow infinite recursive.");
      }
      if (recursive_depth_ < static_cast<std::size_t>(recursive_limit.second)) {
        recursive_depth_++;
        const unsigned char *begin = (const unsigned char*)regex_.c_str(),
            *end = begin + regex_.length();
        Lexer l(begin, end);
        l.Consume();
        e = e0(&l);
        if (recursive_depth_ > static_cast<std::size_t>(recursive_limit.first)) {
          e = new Qmark(e);
        }
        recursive_depth_--;
      } else {
        e = new None();
      }
      return e;
    }
    case Lexer::kRpar:
      exitmsg("expected a '('!");
    case Lexer::kEOP:
      exitmsg("expected none-nullable expression.");
    default:
      exitmsg("can't handle Expr: %s (%c)", lexer->TokenToString(),lexer->literal());
  }

  lexer->Consume();

  return e;
}

CharClass*
Regex::BuildCharClass(Lexer *lexer) {
  CharClass *cc = new CharClass();
  std::bitset<256>& table = cc->table();
  bool range;
  unsigned char lastc = '\0';

  lexer->Consume();
  if (lexer->token() == Lexer::kBegLine) {
    lexer->Consume();
    cc->set_negative(true);
  }
  if (lexer->literal() == '-' ||
      lexer->literal() == ']') {
    table.set(lexer->literal());
    lastc = lexer->literal();
    lexer->Consume();
  }

  for (range = false; lexer->token() != Lexer::kEOP && lexer->literal() != ']'; lexer->Consume()) {
    if (!range && lexer->literal() == '-') {
      range = true;
      continue;
    }

    if (lexer->token() == Lexer::kByteRange) {
      table |= lexer->table();
    } else {
      table.set(lexer->literal());
    }

    if (range) {
      for (std::size_t c = lexer->literal() - 1; c > lastc; c--) {
        table.set(c);
      }
      range = false;
    }

    lastc = lexer->literal();
  }
  if (lexer->token() == Lexer::kEOP) exitmsg(" [ ] imbalance");

  if (range) {
    table.set('-');
  }

  if (cc->count() == 1) {
    lexer->literal(lastc);
  } else if (cc->count() >= 128 && !cc->negative()) {
    cc->set_negative(true);
    cc->flip();
  }
  return cc;
}

// Converte DFA to Regular Expression using GNFA.
// see http://en.wikipedia.org/wiki/Generalized_nondeterministic_finite-state_machine
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
      DFA::state_t next = transition[c];
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
      delete (*iter).second;
      ++iter;
    }
    delete loop;
  }

  if(gnfa_transition[GSTART][GACCEPT] == NULL) {
    return new None();
  } else {
    return gnfa_transition[GSTART][GACCEPT];
  }
}

/*
 *         - slower -
 * Onone: On-The-Fly DFA based matching
 *    O0: DFA based matching
 *      ~ Xbyak(JIT library) required ~
 *    O1: JIT-ed DFA based matching
 *    O2: transition rule optimized-JIT-ed DFA based mathing
 *    O3: transition rule & dfa reduction optimized-JIT-ed DFA based mathing
 *         - faster -
 */

bool Regex::Compile(Regen::Options::CompileFlag olevel) {
  if (olevel == Regen::Options::Onone || olevel_ >= olevel) return true;
  if (!dfa_failure_ && !dfa_.Complete()) {
    /* try create DFA.  */
    std::size_t limit = state_exprs_.size();
    limit = 1000; // default limitation is 1000 (it's may finish within a second).
    dfa_failure_ = !dfa_.Construct(expr_root_, limit);
  }
  if (dfa_failure_) {
    /* can not create DFA. (too many states) */
    return false;
  }

  if (!dfa_.Compile(olevel)) {
    olevel_ = dfa_.olevel();
  } else {
    olevel_ = olevel;
  }
  return olevel_ == olevel;
}

bool Regex::Match(const std::string &string, Regen::Context *context)  const {
  const unsigned char* begin = (const unsigned char *)string.c_str();
  return Match(begin, begin+string.length(), context);
}

bool Regex::Match(const char *begin, const char * end, Regen::Context *context) const {
  return Match((const unsigned char*)begin, (const unsigned char*)end, context);
}

bool Regex::Match(const unsigned char *begin, const unsigned char * end, Regen::Context *context) const {
  return dfa_.Match(begin, end, context);
}

/* Thompson-NFA based matching */
bool Regex::MatchNFA(const unsigned char *begin, const unsigned char *end, Regen::Context *context) const
{
  typedef std::vector<StateExpr*> NFA;
  std::size_t nfa_size = state_exprs_.size();
  std::vector<uint32_t> next_states_flag(nfa_size);
  uint32_t step = 1;
  NFA::iterator iter;
  std::set<StateExpr*>::iterator next_iter;
  NFA states, next_states;
  states.insert(states.begin(), expr_root_->transition().first.begin(), expr_root_->transition().first.end());

  for (const unsigned char *p = begin; p < end; p++, step++) {
    for (iter = states.begin(); iter != states.end(); ++iter) {
      StateExpr *s = *iter;
      if (s->Match(*p)) {
        for (next_iter = s->transition().follow.begin();
             next_iter != s->transition().follow.end();
             ++next_iter) {
          if (next_states_flag[(*next_iter)->state_id()] != step) {
            next_states_flag[(*next_iter)->state_id()] = step;
            next_states.push_back(*next_iter);
          }
        }
      }
    }
    states.swap(next_states);
    if (states.empty()) break;
    next_states.clear();
  }

  bool match = false;
  for (iter = states.begin(); iter != states.end(); ++iter) {
    if ((*iter)->type() == Expr::kEOP) {
      match = true;
      break;
    }
  }

  return match;
}

void Regex::PrintRegex() {
  if (dfa_.Complete()) {
    Expr* e = CreateRegexFromDFA(dfa_);
    PrintRegexVisitor::Print(e);
    delete e;
  } else {
    PrintRegexVisitor::Print(expr_root_);
  }
}

void Regex::PrintParseTree() const {
  PrintParseTreeVisitor::Print(expr_root_);
}

void Regex::DumpExprTree() const {
  DumpExprVisitor::Dump(expr_root_);
}

} // namespace regen
