#include "util.h"
#include "lexer.h"
#include "regen.h"
#include "regex.h"
#include "expr.h"
#include "exprutil.h"
#include "nfa.h"
#include "dfa.h"
#include "sfa.h"
#include "generator.h"

enum Generate { DOTGEN, REGEN, CGEN, TEXTGEN, KEYWORD };

void Dispatch(Generate generate, const regen::DFA &dfa) {
  switch (generate) {
    case CGEN:
      regen::Generator::CGenerate(dfa);
      break;
    case DOTGEN:
      regen::Generator::DotGenerate(dfa);
      break;
    case REGEN:
      regen::Regex::PrintRegex(dfa);
      break;
    case TEXTGEN:
    case KEYWORD:
      break;
  }
}

void ConsumeWS(regen::Lexer &l)
{
  while (l.literal() ==  ' ' ||
         l.literal() == '\t' ||
         l.literal() == '\n') {
    l.Consume();
  }
}

regen::NFA::State& GetState(regen::NFA &nfa, std::size_t state)
{
  if (nfa.size() > state) {
    return nfa[state];
  } else {
    for (std::size_t i = nfa.size(); i < state; i++) {
      nfa.get_new_state();
    }
    return nfa.get_new_state();
  }
}

void ParseStates(regen::Lexer &l, std::set<regen::NFA::state_t> &states)
{
  bool group = false;
  ConsumeWS(l);
  if (l.literal() == '(') group = true;
  do {
    if (group) l.Consume();
    ConsumeWS(l);
    if (!('0' <= l.literal() && l.literal() <= '9')) exitmsg("invalid state description (require digits)");
    regen::NFA::state_t state = 0;
    do {
      state *= 10;
      state += l.literal() - '0';
      l.Consume();
    } while ('0' <= l.literal() && l.literal() <= '9');
    states.insert(state);
    ConsumeWS(l);
  } while (group && l.literal() == ',');
  if (group && l.literal() == ')') l.Consume();
  ConsumeWS(l);
}


void Parse(regen::Lexer &l, regen::NFA &nfa)
{
  ParseStates(l, nfa.start_states());

  if (l.literal() != ',') {
    exitmsg("invalid syntax");
  } else {
    l.Consume();
  }

  std::set<regen::NFA::state_t> states;
  ParseStates(l, states);

  if (l.literal() != ';') {
    exitmsg("invalid syntax");
  } else {
    l.Consume();
  }

  for (std::set<regen::NFA::state_t>::iterator iter = states.begin(); iter != states.end(); ++iter) {
    GetState(nfa, *iter).accept = true;
  }

  std::set<regen::NFA::state_t> dst, src;
  regen::CharClass cc;
  bool dot;
parse_transition:
  dst.clear(); src.clear();
  ParseStates(l, src);
  if (l.literal() != '[') {
    exitmsg("invalid syntax");
  } else if (*l.ptr() == ']') {
    dot = true;
    l.Consume();
    l.Consume();
  } else {
    dot = false;
    cc.table().reset();
    cc.set_negative(false);
    regen::Regex::BuildCharClass(&l, &cc);
    l.Consume();
  }
  ParseStates(l, dst);

  for (std::size_t c = 0; c < 256; c++) {
    for (std::set<regen::NFA::state_t>::iterator iter = src.begin(); iter != src.end(); ++iter) {
      if (dot || cc.Match(c)) {
        GetState(nfa, *iter).transition[c].insert(dst.begin(), dst.end());
      }
    }
  }
  
  if (l.literal() == ',') {
    l.Consume();
    goto parse_transition;
  } else if (l.literal() == '\0') {
    return;
  } else {
    exitmsg("invalid syntax");
  }
}

void die(bool help = false)
{
  printf("USAGE: regen [OPTIONS]* PATTERN\n");
  if (help) {
    printf("Regex selection and interpretation:\n"
           "  -E   PATTERN is an extended regular expression(&,!,&&,||,,,)\n"
           "  -i   ignore case"
           "  -f   obtain PATTERN from FILE\n"
           "Output control:\n"
           "  -t   generate acceptable strings\n"
           "  -d   generate DFA graph (Dot language)\n"
           "  -s   generate SFA graph (Dot language)\n"
           "  -k   extract keywords"
           "  -m   minimizing DFA\n"
           "  -P   partial match mode"
           );
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  std::string regex;
  Regen::Options option(Regen::Options::OneLine);
  bool info, minimize, automata, sfa;
  info = minimize = automata = sfa = false;
  int opt;
  int seed = time(NULL);
  Generate generate = REGEN;

  while ((opt = getopt(argc, argv, "PamdchiIkxEtrsSf:U")) != -1) {
    switch(opt) {
      case 'h':
        die(true);
        break;
      case 'i':
        option.ignore_case(true);
        break;
      case 'I':
        info = true;
        break;
      case 'k':
        generate = KEYWORD;
        option.filtered_match(true);
        break;
      case 'E':
        option.extended(true);
        break;
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
      case 'd':
        generate = DOTGEN;
        break;
      case 'c':
        generate = CGEN;
        break;
      case 'r':
        option.reverse_regex(true);
        break;
      case 'm':
        minimize = true;
        break;
      case 'a':
        automata = true;
        break;
      case 'P':
        option.partial_match(true);
        break;
      case 'S':
        seed = atoi(optarg);
        break;
      case 's':
        sfa = true;
        generate = DOTGEN;
        break;
      case 't': 
        generate = TEXTGEN;
        break;
      case 'U':
        option.encoding_utf8(true);
        break;
      default: die();
    }
  }

  if (regex.empty()) {
    if (optind >= argc) {
      die();
    } else {
      regex = std::string(argv[optind]);
    }
  }

  if (automata) {
    regen::NFA nfa;
    regen::Lexer l((const unsigned char*)regex.c_str(), (const unsigned char*)(regex.c_str()+regex.length()));
    l.Consume();
    Parse(l, nfa);
    regen::DFA dfa(nfa);
    Dispatch(generate, dfa);
    return 0;
  }

  regen::Regex r = regen::Regex(regex, option);

  if (info) {
    printf("%"PRIuS" chars involved. min length = %"PRIuS", max length = %"PRIuS"\n", r.expr_info().involve.count(), r.min_length(), r.max_length());
    return 0;
  } else if (generate == TEXTGEN) {
    srand(seed);
    r.PrintText(regen::Expr::GenAll);
  } else if (generate == REGEN && !minimize) {
    r.PrintRegex();
    return 0;
  } else if (generate == KEYWORD) {
    printf("is: %s\nleft: %s\nright: %s\nin:\n", r.expr_info().key.is.data(),
           r.expr_info().key.left.data(), r.expr_info().key.right.data());
    for (std::set<std::string>::iterator iter = r.expr_info().key.in.begin();
         iter != r.expr_info().key.in.end(); ++iter) {
      printf("%s\n", iter->data());
    }
    puts("candidates:");
    for (std::set<std::string>::iterator iter = r.expr_info().key.candidates.begin();
         iter != r.expr_info().key.candidates.end(); ++iter) {
      printf("%s\n", iter->data());
    }
  } else {
    r.Compile(Regen::Options::O0);
    if (minimize) r.MinimizeDFA();
  }

  if (generate == REGEN) {
    r.PrintRegex();
  } else {
#ifdef REGEN_ENABLE_PARALLEL
    if (sfa) {
      regen::SFA s(r.dfa());
      Dispatch(generate, s);
    } else
#endif //REGEN_ENABLE_PARALLEL
    {
      Dispatch(generate, r.dfa());
    }
  }
  
  return 0;
}
