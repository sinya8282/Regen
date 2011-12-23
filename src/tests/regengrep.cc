#include "../regen.h"
#include "../util.h"
#include <unistd.h>

struct Option {
  Option(): count_line(false), only_matching(false), print_file(0), filename(NULL), pflag(Regen::Options::ShortestMatch | Regen::Options::PartialMatch), olevel(Regen::Options::O3) {}
  bool count_line;
  bool only_matching;
  int print_file;
  const char *filename;
  Regen::Options pflag;
  Regen::Options::CompileFlag olevel;
};

void grep(const Regen &re, const regen::Util::mmap_t &buf, const Option &opt);

const char* get_line_beg(const char* buf, const char *beg)
{
  while (buf > beg && *buf != '\n') buf--;
  return buf;
}

int main(int argc, char *argv[])
{
  std::string regex;
  Option opt;
  int opt_;

  while ((opt_ = getopt(argc, argv, "chHf:oO:")) != -1) {
    switch(opt_) {
      case 'c':
        opt.count_line = true;
        break;
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
      case 'h':
        opt.print_file = -1;
        break;
      case 'H':
        opt.print_file = 1;
        break;
      case 'o':
        opt.only_matching = true;
        opt.pflag.captured_match(true);
        break;
      case 'O':
        opt.olevel = Regen::Options::CompileFlag(atoi(optarg));
        break;
    }
  }

  if (opt.count_line) {
    opt.only_matching = false;
    opt.pflag.captured_match(false);
  }
  
  if (regex.empty()) {
    if (optind+1 >= argc) {
      exitmsg("USAGE: regen [options] regexp file\n");
    } else {
      regex = std::string(argv[optind++]);
    }
  } else if (optind >= argc) {
    exitmsg("USAGE: regen [options] regexp file\n");
  }

  Regen re(regex, opt.pflag);
  re.Compile(opt.olevel);

  if (optind < argc+1 && opt.print_file != -1) opt.print_file = 1;
  
  for (int i = optind; i < argc; i++) {
    opt.filename = argv[optind];
    regen::Util::mmap_t buf(opt.filename);
    grep(re, buf, opt);
  }

  return 0;
}

void grep(const Regen &re, const regen::Util::mmap_t &buf, const Option &opt)
{
  const char *str = (char*)buf.ptr;
  const char *end = str + buf.size;
  static const char newline[] = "\n";
  int count = 0;
  Regen::Context context;
  while (re.Match(str, end, &context)) {
    if (opt.only_matching) {
      write(1, context.begin(), context.end()-context.begin());
      write(1, newline, 1);
      str = context.end();
    } else {
      const char *end_ = (const char*)memchr(context.end(), '\n', end-context.end());
      if (end_ == NULL) end_ = end;
      if (opt.count_line) {
        count++;
      } else {
        const char *beg = get_line_beg(context.end(), str);
        write(1, beg, end_-beg+1);
      }
      str = end_+1;
    }
    if (str >= end) break;
  }
  if (opt.count_line) printf("%d\n", count);
}
