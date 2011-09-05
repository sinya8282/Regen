#ifndef REGEN_UTIL_H_
#define REGEN_UTIL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <cassert>
#include <cstddef>
#include <limits>
#include <string>
#include <set>
#include <bitset>
#include <vector>
#include <stack>
#include <queue>
#include <deque>
#include <functional>
#include <map>
#include <iostream>
#include <typeinfo>
#include <fstream>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#ifdef _LP64
#define __PRIS_PREFIX "z"
#else
#define __PRIS_PREFIX
#endif

#define PRIdS __PRIS_PREFIX "d"
#define PRIxS __PRIS_PREFIX "x"
#define PRIuS __PRIS_PREFIX "u"
#define PRIXS __PRIS_PREFIX "X"
#define PRIoS __PRIS_PREFIX "o"

#define errmsg(format, args...) fprintf(stderr, "in %s: "format, __FUNCTION__, ## args)
#define exitmsg(format, args...) { errmsg(format, ## args); exit(-1); }
#define DISALLOW_COPY_AND_ASSIGN(TypeName) TypeName(const TypeName&); void operator=(const TypeName&)

namespace regen {

enum CompileFlag {
  Onone = -1,
  O0 = 0,
  O1 = 1,
  O2 = 2,
  O3 = 3
};
  
namespace Util {
  
struct mmap_t{
  mmap_t(const char* path, bool write_mode=false, int flags=MAP_FILE|MAP_PRIVATE){
    int OPEN_MODE=O_RDONLY;
    int PROT = PROT_READ;
    if(write_mode) {
      OPEN_MODE=O_RDWR;
      PROT |= PROT_WRITE;
    }

    int f = open(path, OPEN_MODE);
    struct stat statbuf;
    fstat(f, &statbuf);
    ptr = (unsigned char *)mmap(0, statbuf.st_size, PROT, flags, f, 0);
    size=statbuf.st_size;
    close(f);
  }

  ~mmap_t() {
    munmap(ptr, size);
  }

  operator bool () const
  { return ptr != (void *)-1; }

  size_t size;
  unsigned char *ptr;
};

} // namespace Util

} // namespace regen

#endif // REGEN_UTIL_H_
