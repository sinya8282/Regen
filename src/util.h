#ifndef REGEN_UTIL_H_
#define REGEN_UTIL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(_MSC_VER) && (_MSC_VER <= 1500)
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif
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
#ifdef _MSC_VER
#include <windows.h>
#include "win/getopt.h"
#else
#include <sys/mman.h>
#include <fcntl.h>
#endif

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

#ifdef _MSC_VER
#define errmsg(format, ...) fprintf(stderr, "in %s: "format, __FUNCTION__, ## __VA_ARGS__)
#define exitmsg(format, ...) { errmsg(format, ## __VA_ARGS__); exit(-1); }
#else
#define errmsg(format, args...) fprintf(stderr, "in %s: "format, __FUNCTION__, ## args)
#define exitmsg(format, args...) { errmsg(format, ## args); exit(-1); }
#endif
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

#ifdef _WIN32
struct mmap_t{
  mmap_t(const char* path, bool write_mode=false, int flags=0)
    : size(0)
    , ptr(0)
    , h1(0)
    , h2(0)
  {
    if (write_mode || flags) {
      fprintf(stderr, "not support mode write_mode=%d, flags=%d\n", write_mode, flags);
    }
    h1 = ::CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h1 == INVALID_HANDLE_VALUE) {
      fprintf(stderr, "ERR:CreateFile %s\n", path);
      exit(1);
    }
    LARGE_INTEGER li;
    if (::GetFileSizeEx(h1, &li) == 0) {
      fprintf(stderr, "ERR:GetFileSizeEx %s\n", path);
      exit(1);
    }
    size = li.QuadPart;
    h2 = ::CreateFileMapping(h1, NULL, PAGE_READONLY, 0, 0, NULL);
    if (h2 == NULL) {
      fprintf(stderr, "ERR:CreateFileMapping %s\n", path);
      exit(1);
    }
    ptr = reinterpret_cast<unsigned char*>(MapViewOfFile(h2, FILE_MAP_READ, 0, 0, 0));
    if (ptr == NULL) {
      fprintf(stderr, "ERR:MapViewOfFile\n", path);
      exit(1);
    }
  }

  ~mmap_t() {
    if (!ptr) return;
    UnmapViewOfFile(ptr);
    CloseHandle(h2);
    CloseHandle(h1);
  }

  operator bool () const
  { return ptr != (void *)-1; }

  uint64_t size;
  unsigned char *ptr;
  HANDLE h1, h2;
};
#else
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
#endif

} // namespace Util

} // namespace regen

#endif // REGEN_UTIL_H_
