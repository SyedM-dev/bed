#pragma once

#include "pch.h"

inline int read_file(const char *path, char **text, uint32_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return 0;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (size < 0 || size > UINT32_MAX) {
    fclose(f);
    return 0;
  }

  *len = (uint32_t)size;

  *text = (char *)malloc(*len + 1);
  if (!*text) {
    fclose(f);
    return 0;
  }

  size_t read = fread(*text, 1, *len, f);
  fclose(f);

  if (read != *len) {
    free(*text);
    *text = NULL;
    *len = 0;
    return 0;
  }

  (*text)[*len] = '\0'; // handy if treating it as a C string

  return 1;
}
