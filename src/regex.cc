#include "regex.h"

namespace regen {

Regex::Regex(const Regen::StringPiece& pattern, const Regen::Options flags):
    regex_(pattern.as_string()),
    flag_(flags),
    recursion_depth_(0),
    involved_char_(std::bitset<256>()),
    olevel_(Regen::Options::Onone),
    dfa_failure_(false),
    dfa_(flags)
{
  Parse();
  dfa_.set_expr_info(expr_info_);
}

StateExpr* Regex::CombineStateExpr(StateExpr *e1, StateExpr *e2, ExprPool *p)
{
  StateExpr *s;
  CharClass *cc = p->alloc<CharClass>(e1, e2);
  if (cc->count() == 256) {
    //delete cc;
    s = p->alloc<Dot>();
  } else if (cc->count() == 1) {
    //delete cc;
    char c;
    switch (e1->type()) {
      case Expr::kLiteral:
        c = ((Literal*)e1)->literal();
        break;
      default: exitmsg("Invalid Expr Type: %d", e1->type());
    }
    s = p->alloc<Literal>(c);
  } else {
    s = cc;
  }
  return s;
}

Expr* Regex::PatchBackRef(Lexer *lexer, Expr *e, ExprPool *p)
{
  std::set<std::size_t> &backrefs = lexer->backrefs();
  for (std::set<std::size_t>::iterator iter = backrefs.begin();
       iter != backrefs.end(); ++iter) {
    std::size_t ref_id = *iter;
    Expr* referee = lexer->groups()[ref_id];
    Operator *patch = p->alloc<Operator>(Operator::kBackRef);
    patch->set_id(ref_id);
    patch->set_parent(referee->parent());
    switch (referee->parent()->type()) {
      case Expr::kConcat: case Expr::kUnion:
      case Expr::kIntersection: case Expr::kXOR: {
        BinaryExpr* p = static_cast<BinaryExpr*>(referee->parent());
        if (p->lhs() == referee) {
          p->set_lhs(patch);
        } else if (p->rhs() == referee) {
          p->set_rhs(patch);
        } else {
          exitmsg("inconsistency parent-child pointer");
        }
        break;
      }
      case Expr::kQmark: case Expr::kStar: case Expr::kPlus: {
        UnaryExpr *p = static_cast<UnaryExpr*>(referee->parent());
        if (p->lhs() == referee) {
          p->set_lhs(patch);
        } else {
          exitmsg("inconsistency parent-child pointer");
        }
        break;
      }
      default: exitmsg("invalid types");
    }

    std::vector<Expr*> patches;
    referee->Serialize(patches, p);
    //delete referee;
    std::vector<Expr*> roots;
    roots.push_back(e);
    while (patches.size() > roots.size()) {
      roots.push_back(e->Clone(p));
    }
    for (std::size_t i = 0; i < roots.size(); i++) {
      roots[i]->PatchBackRef(patches[i], ref_id, p);
    }
    e = roots[0];
    for (std::size_t i = 1; i < roots.size(); i++) {
      e = p->alloc<Union>(e, roots[i]);
    }
  }

  return e;
}

void Regex::Parse()
{
  const unsigned char *begin = (const unsigned char*)regex_.c_str(),
      *end = begin + regex_.length();
  Lexer lexer(begin, end, flag_);
  lexer.Consume();
  Expr* e;

  e = e0(&lexer, &pool_);
  if (e->type() == Expr::kNone) exitmsg("Inavlid pattern.");
  if (lexer.token() != Lexer::kEOP) exitmsg("Expected end of pattern.");

  if (!lexer.backrefs().empty()) e = PatchBackRef(&lexer, e, &pool_);

  expr_info_.orig_root = e;

  e->set_nonnullable(flag_.non_nullable());

  if (flag_.filtered_match()) e->FillKeywords(&expr_info_.key, &expr_info_.involve);
  
  if (!flag_.prefix_match()) {
    // rewrite expression R when Prefix-free Matching is required
    // R -> .*?R
    Expr *dotstar = pool_.alloc<Star>(pool_.alloc<Dot>(), true);
    e = pool_.alloc<Concat>(dotstar, e, flag_.reverse_regex());
  }

  expr_info_.eop = pool_.alloc<EOP>();
  e = pool_.alloc<Concat>(e, expr_info_.eop);

  expr_info_.expr_root = e;
  e->FillPosition(&expr_info_);
  expr_info_.min_length = expr_info_.orig_root->min_length();
  expr_info_.max_length = expr_info_.orig_root->max_length();
  e->FillTransition();
}

/* Regen parsing rules
 * RE ::= e0 EOP
 * e0 ::= e1 ('||' e1)*                   # shuffle
 * e1 ::= e2 ('&&' e2)*                   # xor
 * e2 ::= e3 ('|' e3)*                    # union
 * e3 ::= e4 ('&' e4)*                    # intersection
 * e4 ::= e5+                             # concatenation
 * e5 ::= e6 ([?+*]|{N,N}|{,}|{,N}|{N,})* # repetition
 * e6 ::= ATOM | '(' e0 ')' | '!' e0 | '#' e0 # ATOM, grouped, complement, permutation
*/

Expr* Regex::e0(Lexer *lexer, ExprPool *pool)
{
  Expr *e, *f;
  e = e1(lexer, pool);

  while (lexer->token() == Lexer::kShuffle) {
    lexer->Consume();
    f = e1(lexer, pool);
    e = Expr::Shuffle(e, f, pool);
  }

  return e;
}

Expr* Regex::e1(Lexer *lexer, ExprPool *pool)
{
  Expr *e, *f;
  e = e2(lexer, pool);

  while (lexer->token() == Lexer::kXOR) {
    lexer->Consume();
    f = e2(lexer, pool);
    e = pool->alloc<XOR>(e, f, pool);
  }

  return e;
}

Expr* Regex::e2(Lexer *lexer, ExprPool *pool)
{
  Expr *e, *f;
  e = e3(lexer, pool);

  while (lexer->token() == Lexer::kUnion) {
    lexer->Consume();
    f = e3(lexer, pool);
    e = pool->alloc<Union>(e, f);
  }

  return e;
}

Expr* Regex::e3(Lexer *lexer, ExprPool *pool)
{
  Expr *e, *f;
  e = e4(lexer, pool);

  while (lexer->token() == Lexer::kIntersection) {
    lexer->Consume();
    f = e4(lexer, pool);
    e = pool->alloc<Intersection>(e, f, pool);
  }
  
  return e;
}

Expr* Regex::e4(Lexer *lexer, ExprPool *pool)
{
  Expr *e, *f;
  e = e5(lexer, pool);

  while (lexer->Concatenated()) {
    f = e5(lexer, pool);
    e = pool->alloc<Concat>(e, f, flag_.reverse_regex());
  }

  return e;
}

Expr* Regex::e5(Lexer *lexer, ExprPool *pool)
{
  Expr *e;
  e = e6(lexer, pool);

  while (lexer->Quantifier()) {
    bool non_greedy = false;
    Lexer::Type token = lexer->token();
    double probability = lexer->probability();
    lexer->Consume();
    if (lexer->token() == Lexer::kQmark) {
      non_greedy = true;
      lexer->Consume();
    }
    switch (token) {
      case Lexer::kStar: {
        e = pool->alloc<Star>(e, non_greedy, probability);
        break;
      }
      case Lexer::kPlus: {
        if (non_greedy) {
          Expr* e_ = e->Clone(pool);
          e = pool->alloc<Concat>(e_, pool->alloc<Star>(e, non_greedy, probability), flag_.reverse_regex());
        } else {
          e = pool->alloc<Plus>(e, probability);
        }
        break;
      }
      case Lexer::kQmark: {
        e = pool->alloc<Qmark>(e, non_greedy, probability);
        break;
      }
      case Lexer::kRepetition: {
        std::pair<int, int> r = lexer->repetition();
        int lower_repetition = r.first, upper_repetition = r.second;
        if (lower_repetition == 0 && upper_repetition == 0) {
          //delete e;
          e = pool->alloc<Epsilon>();
        } else if (upper_repetition == -1) {
          Expr* f = e;
          for (int i = 0; i < lower_repetition - 1; i++) {
            e = pool->alloc<Concat>(e, f->Clone(pool), flag_.reverse_regex());
          }
          e = pool->alloc<Concat>(e, pool->alloc<Star>(f->Clone(pool), non_greedy, probability), flag_.reverse_regex());
        } else if (upper_repetition == lower_repetition) {
          Expr *f;
          if (probability == 0.0) {
            f = e;
          } else {
            f = pool->alloc<Qmark>(e, non_greedy, probability);
          }
          for (int i = 0; i < lower_repetition - 1; i++) {
            e = pool->alloc<Concat>(e, f->Clone(pool), flag_.reverse_regex());
          }
        } else {
          Expr *f = e;
          for (int i = 0; i < lower_repetition - 1; i++) {
            e = pool->alloc<Concat>(e, f->Clone(pool), flag_.reverse_regex());
          }
          if (lower_repetition == 0) {
            e = pool->alloc<Qmark>(e, non_greedy, probability);
            lower_repetition++;
          }
          for (int i = 0; i < (upper_repetition - lower_repetition); i++) {
            e = pool->alloc<Concat>(e, pool->alloc<Qmark>(f->Clone(pool), non_greedy, probability), flag_.reverse_regex());
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

std::size_t UTF8ByteLength(const unsigned char c)
{
  static const std::size_t len[] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,
    0,0,0,0,0,0,0,0
  };
  return len[c];
}

bool IsReagalUTF8Sequence(const unsigned char *s)
{
  std::size_t len = UTF8ByteLength(*s);
  if (len == 0) return false;
  for (std::size_t i = 1; i < len; i++) {
    if (!(0x80 <= *(s+i) && *(s+i) < 0xC0)) return false;
  }
  return true;
}

Expr* Regex::e6(Lexer *lexer, ExprPool *pool)
{ 
  Expr *e;

  switch(lexer->token()) {
    case Lexer::kLiteral:
      if (flag_.encoding_utf8()) {
        std::size_t len = UTF8ByteLength(lexer->literal());
        if (IsReagalUTF8Sequence(lexer->ptr()-1)) {
          e = pool->alloc<Literal>(lexer->literal());
          while (--len > 0) {
            lexer->Consume();
            e = pool->alloc<Concat>(pool->alloc<Literal>(lexer->literal()), e, flag_.reverse_regex());
          }
        } else {
          exitmsg("Invalid UTF8 byte sequence.");
        }
      } else {
        e = pool->alloc<Literal>(lexer->literal());
      }
      break;
    case Lexer::kBegLine:
      e = pool->alloc<Anchor>(Anchor::kBegLine);
      break;
    case Lexer::kEndLine:
      e = pool->alloc<Anchor>(Anchor::kEndLine);
      break;
    case Lexer::kDot:
      e = pool->alloc<Dot>();
      break;
    case Lexer::kByteRange: {
      e = pool->alloc<CharClass>(lexer->table());
      break;
    }
    case Lexer::kCharClass: {
      CharClass *cc = pool->alloc<CharClass>();
      BuildCharClass(lexer, cc);
      if (cc->count() == 1) {
        e = pool->alloc<Literal>(lexer->literal());
        //delete cc;
      } else if (cc->count() == 256) {
        e = pool->alloc<Dot>();
        //delete cc;
      } else {
        e = cc;
      }
      break;
    }
    case Lexer::kNone:
      e = pool->alloc<None>();
      break;
    case Lexer::kEpsilon:
      e = pool->alloc<Epsilon>();
      break;
    case Lexer::kBackRef: {
      if (lexer->backref() >= lexer->groups().size()) exitmsg("Invalid back reference");
      if (lexer->weakref()) {
        e = lexer->groups()[lexer->backref()]->Clone(pool);
      } else {
        lexer->backrefs().insert(lexer->backref());
        Operator *o = pool->alloc<Operator>(Operator::kBackRef);
        o->set_id(lexer->backref());
        e = o;
      }
      break;
    }
    case Lexer::kLpar: {
      lexer->Consume();
      std::size_t ngroup = lexer->groups().size();
      lexer->groups().push_back(0);
      if (lexer->token() == Lexer::kRpar) {
        e = pool->alloc<Epsilon>();
      } else {
        e = e0(lexer, pool);
      }
      if (lexer->token() != Lexer::kRpar) exitmsg("expected a ')'");
      lexer->groups()[ngroup] = e;
      break;
    }
    case Lexer::kComplement: {
      bool complement = false;
      do {
        complement = !complement;
        lexer->Consume();
      } while (lexer->token() == Lexer::kComplement);
      e = e6(lexer, pool);
      if (complement) {
        if (e->type() == Expr::kNone) {
          //delete e;
          return pool->alloc<Star>(pool->alloc<Dot>());
        }
        Expr *dotstar = pool->alloc<Star>(pool->alloc<Dot>());
        e = pool->alloc<XOR>(dotstar, e, pool); /* R xor .* == !R */
      }
      return e;
    }
    case Lexer::kPermutation: {
      lexer->Consume();
      ExprPool tmp_pool;
      e = e6(lexer, &tmp_pool);
      e = Expr::Permutation(e, pool);
      return e;
    }
    case Lexer::kReverse: {
      lexer->Consume();
      bool reverse = flag_.reverse_regex();
      flag_.reverse_regex(!reverse);
      e = e6(lexer, pool);
      flag_.reverse_regex(reverse);
      return e;
    }
    case Lexer::kRecursion: {
      lexer->Consume();
      std::pair<int, int> recursion_limit;
      recursion_limit.first = recursion_limit.second = 1;
      if (lexer->token() == Lexer::kRepetition) {
        recursion_limit = lexer->repetition();
        lexer->Consume();
      }
      if (recursion_limit.second == -1) {
        exitmsg("disallow infinite recursion.");
      }
      if (recursion_depth_ < static_cast<std::size_t>(recursion_limit.second)) {
        recursion_depth_++;
        Lexer l(lexer->begin(), lexer->end(), flag_);
        l.Consume();
        e = e0(&l, pool);
        if (recursion_depth_ > static_cast<std::size_t>(recursion_limit.first)) {
          e = pool->alloc<Qmark>(e);
        }
        recursion_depth_--;
      } else {
        e = pool->alloc<Epsilon>();
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

CharClass* Regex::BuildCharClass(Lexer *lexer, CharClass *cc)
{
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
void Regex::CreateRegexFromDFA(const DFA &dfa, ExprInfo *info, ExprPool *p)
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
            e = p->alloc<Dot>();
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
            e = p->alloc<CharClass>(table, negative);
          }
        } else {
          e = p->alloc<Literal>(c);
        }
        if (gtransition.find(next) != gtransition.end()) {
          Expr* f = gtransition[next];
          if (Expr::SuperTypeOf(e) == Expr::kStateExpr &&
              Expr::SuperTypeOf(f) == Expr::kStateExpr) {
            Expr *e_ = CombineStateExpr((StateExpr*)e, (StateExpr*)f, p);
            //delete e;
            //delete f;
            e = e_;
          } else {
            e = p->alloc<Union>(e, f);
          }
        }
        gtransition[next] = e;
      }
    }
  }

  for (std::size_t i = 0; i < dfa.size(); i++) {
    if (dfa.IsAcceptOrEndlineState(i)) {
      if (dfa.IsEndlineState(i)) {
        gnfa_transition[i][GACCEPT] = p->alloc<Anchor>(Anchor::kEndLine);
      } else {
        gnfa_transition[i][GACCEPT] = NULL;
      }
    }
  }

  gnfa_transition[GSTART][0] = NULL;
  
  for (int i = 0; i < GSTART; i++) {
    Expr* loop = NULL;
    GNFATrans &gtransition = gnfa_transition[i];
    if (gtransition.find(i) != gtransition.end()) {
      loop = p->alloc<Star>(gtransition[i]);
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
              regex2 = p->alloc<Concat>(loop, regex2);
            } else {
              regex2 = loop;
            }
          }
          if (regex1 != NULL) {
            if (regex2 != NULL) {
              regex2 = p->alloc<Concat>(regex1, regex2);
            } else {
              regex2 = regex1;
            }
          }
          if (regex2 != NULL) {
            regex2 = regex2->Clone(p);
          }
          if (gnfa_transition[j].find((*iter).first) != gnfa_transition[j].end()) {
            if (gnfa_transition[j][(*iter).first] != NULL) {
              if (regex2 != NULL) {
                Expr* e = gnfa_transition[j][(*iter).first];
                Expr* f = regex2;
                if (Expr::SuperTypeOf(e) == Expr::kStateExpr &&
                    Expr::SuperTypeOf(f) == Expr::kStateExpr) {
                  Expr *e_ = CombineStateExpr((StateExpr*)e, (StateExpr*)f, p);
                  //delete e;
                  //delete f;
                  e = e_;
                } else {
                  e = p->alloc<Union>(e, f);
                }
                gnfa_transition[j][(*iter).first] = e;
              } else {
                gnfa_transition[j][(*iter).first] =
                    p->alloc<Qmark>(gnfa_transition[j][(*iter).first]);
              }
            } else {
              if (regex2 != NULL) {
                gnfa_transition[j][(*iter).first] = p->alloc<Qmark>(regex2);
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
  }

  if(gnfa_transition[GSTART][GACCEPT] == NULL) {
    info->expr_root = p->alloc<None>();
  } else {
    info->eop = p->alloc<EOP>();
    info->expr_root = p->alloc<Concat>(gnfa_transition[GSTART][GACCEPT], info->eop);
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
    dfa_failure_ = !dfa_.Construct(limit);
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

bool Regex::Match(const Regen::StringPiece& string, Regen::StringPiece *result)  const {
  return dfa_.Match(string, result);
}

/* Thompson-NFA based matching */
bool Regex::NFAMatch(const Regen::StringPiece& string, Regen::StringPiece *result) const
{
  typedef std::vector<StateExpr*> NFA;
  std::size_t nfa_size = state_exprs_.size();
  std::vector<uint32_t> next_states_flag(nfa_size);
  uint32_t step = 1;
  NFA::iterator iter;
  std::set<StateExpr*>::iterator next_iter;
  NFA states, next_states;
  states.insert(states.begin(), expr_info_.expr_root->transition().first.begin(), expr_info_.expr_root->transition().first.end());

  for (const unsigned char *p = string.ubegin(); p < string.uend(); p++, step++) {
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

void Regex::PrintRegex() const
{
  if (dfa_.Complete()) {
    ExprPool p;
    ExprInfo i;
    CreateRegexFromDFA(dfa_, &i, &p);
    PrintRegexVisitor::Print(i.expr_root);
  } else {
    PrintRegexVisitor::Print(expr_info_.expr_root);
  }
}

void Regex::PrintRegex(const DFA &dfa)
{
  ExprPool p;
  ExprInfo i;
  CreateRegexFromDFA(dfa, &i, &p);
  PrintRegexVisitor::Print(i.expr_root);
}

void Regex::PrintParseTree() const
{
  PrintParseTreeVisitor::Print(expr_info_.expr_root);
}

void Regex::DumpExprTree() const
{
  DumpExprVisitor::Dump(expr_info_.expr_root);
}

void Regex::PrintText(Expr::GenOpt opt, std::size_t n) const
{
  std::set<std::string> g;
  expr_info_.expr_root->Generate(g, opt, n);
  for (std::set<std::string>::iterator iter = g.begin(); iter != g.end(); ++iter) {
    printf("%s\n", iter->c_str());
  }
}

} // namespace regen
