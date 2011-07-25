#include "../util.h"

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

int main(int argc, char *argv[])
{
  int x = 0;
  uint64_t time = 0;
  regen::Util::mmap_t mm(argv[1]);

  time -= rdtsc();
  for (const unsigned char *p = mm.ptr; p < mm.ptr+mm.size; p++) x += *p;
  time += rdtsc();
  printf("%d\n", time);

  return x;
}
