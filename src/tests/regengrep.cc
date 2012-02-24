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

  while ((opt_ = getopt(argc, argv, "cf:hHoiO:qU")) != -1) {
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
      case 'i':
        opt.pflag.ignore_case(true);
        break;
      case 'H':
        opt.print_file = 1;
        break;
      case 'o':
        opt.only_matching = true;
        opt.pflag.longest_match(true);
        opt.pflag.captured_match(true);
        break;
      case 'O':
        opt.olevel = Regen::Options::CompileFlag(atoi(optarg));
        break;
      case 'q':
        opt.pflag.filtered_match(true);
        break;
      case 'U':
        opt.pflag.encoding_utf8(true);
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
  Regen::StringPiece string(buf.ptr, buf.size), result;
  static const char newline[] = "\n";
  int count = 0;
  while (re.Match(string, &result)) {
    if (opt.only_matching) {
      if (result.begin() == result.end()) {
        string.set_begin(result.end() + 1);
      } else {
        write(1, result.begin(), result.size());
        write(1, newline, 1);
        string.set_begin(result.end());
      }
    } else {
      const char *end = (const char*)memchr(result.end(), '\n', string.end()-result.end());
      if (end == NULL) end = string.end();
      if (opt.count_line) {
        count++;
      } else {
        const char *beg = get_line_beg(result.end(), string.begin());
        write(1, beg, end-beg+1);
      }
      string.set_begin(end+1);
    }
    if (string.empty()) break;
  }
  if (opt.count_line) printf("%d\n", count);
}
