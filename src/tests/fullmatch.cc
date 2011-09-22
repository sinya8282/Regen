#include "../regex.h"
#include "../ssfa.h"

static inline uint64_t rdtsc()
{
  #ifdef _MSC_VER
  return __rdtsc();
  #else
  unsigned int eax, edx;
  __asm__ volatile("rdtsc" : "=a"(eax), "=d"(edx));
  return ((uint64_t)edx << 32) | eax;
  #endif
}

int main(int argc, char *argv[]) {
  std::string regex;
  int opt;
  std::size_t thread_num = 1;
  std::size_t count = 1;
  regen::CompileFlag olevel = regen::Onone;

  while ((opt = getopt(argc, argv, "c:f:O:t:")) != -1) {
    switch(opt) {
      case 'c': {
        count = atoi(optarg);
        break;
      }
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
      case 'O': {
        olevel = regen::CompileFlag(atoi(optarg));
        break;
      }
      case 't': {
        thread_num = atoi(optarg);
        break;
      }
    }
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

  regen::Util::mmap_t mm(argv[optind]);

  for (std::size_t i = 0; i < count; i++) {
    uint64_t compile_time = 0, matching_time = 0;
    bool match = false;

    if (thread_num <= 1) {
      compile_time -= rdtsc();
      regen::Regex r = regen::Regex(regex);
      r.Compile(olevel);
      compile_time += rdtsc();
      matching_time -= rdtsc();
      match = r.Match(mm.ptr, mm.ptr+mm.size);
      matching_time += rdtsc();
    } else {
      compile_time -= rdtsc();
      regen::Regex r = regen::Regex(regex);
      r.Compile(regen::O0);
      regen::SSFA ssfa(r.expr_root(), r.state_exprs(), thread_num);
      ssfa.Compile(olevel);
      compile_time += rdtsc();
      matching_time -= rdtsc();
      match = ssfa.Match(mm.ptr, mm.ptr+mm.size);
      matching_time += rdtsc();
    }

    printf("compile time = %llu, matching time = %llu, %s\n",
           compile_time, matching_time, match ? "match" : "not match." );
  }
  
  return 0;
}
