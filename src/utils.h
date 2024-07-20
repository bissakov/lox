#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include "../src/main.h"

template <typename T>
T *GetElement(T *elements, int length, int idx) {
  Assert(idx >= 0);
  Assert(idx < length);
  T *element = &elements[idx];
  return element;
}

const char *ToString(enum TokenType token_type);

#endif  // SRC_UTILS_H_
