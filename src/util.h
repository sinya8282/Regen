#ifndef REGEN_UTIL_H_
#define REGEN_UTIL_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstddef>
#include <string>
#include <set>
#include <bitset>
#include <vector>
#include <queue>
#include <deque>
#include <functional>
#include <map>
#include <iostream>
#include <typeinfo>
#include <fstream>

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
#define exitmsg(format, args...) { errmsg(format, ## args), exit(-1); }
#define DISALLOW_COPY_AND_ASSIGN(TypeName) TypeName(const TypeName&); void operator=(const TypeName&)

namespace regen {

template<typename T>
struct DeleteObject:
    public std::unary_function<const T*, void> {
  void operator()(const T* ptr) const
  {
    delete ptr;
  }
};

} // namespace regen

#endif // REGEN_UTIL_H_
