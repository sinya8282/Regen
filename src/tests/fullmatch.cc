#include "../regex.h"
#include "../paralleldfa.h"

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
  regen::Optimize olevel = regen::Onone;

  while ((opt = getopt(argc, argv, "f:O:t:")) != -1) {
    switch(opt) {
      case 'f': {
        std::ifstream ifs(optarg);
        ifs >> regex;
        break;
      }
      case 'O': {
        olevel = regen::Optimize(atoi(optarg));
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
  regen::Regex r = regen::Regex(regex);
  uint64_t compile_time = 0, matching_time = 0;
  bool match = false;
  
  if (thread_num <= 1) {
    compile_time -= rdtsc();
    r.Compile(olevel);
    compile_time += rdtsc();
    matching_time -= rdtsc();
    match = r.FullMatch(mm.ptr, mm.ptr+mm.size);
    matching_time += rdtsc();
  } else {
    compile_time -= rdtsc();
    regen::ParallelDFA pdfa(r.expr_root(), r.state_exprs(), thread_num);
    pdfa.Compile(olevel);
    compile_time += rdtsc();
    matching_time -= rdtsc();
    match = pdfa.FullMatch(mm.ptr, mm.ptr+mm.size);
    matching_time += rdtsc();
  }

  printf("compile time = %llu, matching time = %llu, %s\n",
         compile_time, matching_time, match ? "match" : "not match." );
  
  return 0;
}
