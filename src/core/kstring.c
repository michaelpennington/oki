#include "core/kstring.h"
#include "core/kmemory.h"

char *string_duplicate(const char *str) {
  u64 length = string_length(str);
  char *copy = kallocate(length + 1, MEMORY_TAG_STRING);
  kcopy_memory(copy, str, length + 1);
  return copy;
}

u64 string_length(const char *str) {
  const char *s = str;
  while (*(++s)) {
  }
  return s - str;
}

bool strings_equal(const char *str0, const char *str1) {
  for (u64 i = 0;; i++) {
    if (str0[i]) {
      if (str0[i] != str1[i]) {
        return false;
      }
    } else {
      return !str1[i];
    }
  }
}
