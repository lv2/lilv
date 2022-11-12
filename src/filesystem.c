// Copyright 2007-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "filesystem.h"
#include "lilv_config.h"
#include "lilv_internal.h"

#include "zix/allocator.h"
#include "zix/filesystem.h"
#include "zix/path.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
lilv_is_dir_sep(const char c)
{
  return c == '/' || c == LILV_DIR_SEP[0];
}

#ifdef _WIN32
static inline bool
is_windows_path(const char* path)
{
  return (isalpha(path[0]) && (path[1] == ':' || path[1] == '|') &&
          (path[2] == '/' || path[2] == '\\'));
}
#endif

bool
lilv_path_is_child(const char* path, const char* dir)
{
  if (path && dir) {
    const size_t path_len = strlen(path);
    const size_t dir_len  = strlen(dir);
    return dir && path_len >= dir_len && !strncmp(path, dir, dir_len);
  }
  return false;
}

char*
lilv_path_parent(const char* path)
{
  const char* s = path + strlen(path) - 1; // Last character

  // Last non-slash
  for (; s > path && lilv_is_dir_sep(*s); --s) {
  }

  // Last internal slash
  for (; s > path && !lilv_is_dir_sep(*s); --s) {
  }

  // Skip duplicates
  for (; s > path && lilv_is_dir_sep(*s); --s) {
  }

  if (s == path) { // Hit beginning
    return lilv_is_dir_sep(*s) ? lilv_strdup("/") : lilv_strdup(".");
  }

  // Pointing to the last character of the result (inclusive)
  char* dirname = (char*)malloc(s - path + 2);
  memcpy(dirname, path, s - path + 1);
  dirname[s - path + 1] = '\0';
  return dirname;
}

char*
lilv_create_temporary_directory(const char* pattern)
{
  char* const tmpdir       = zix_temp_directory_path(NULL);
  char* const path_pattern = zix_path_join(NULL, tmpdir, pattern);
  char* const result       = zix_create_temporary_directory(NULL, path_pattern);

  zix_free(NULL, path_pattern);
  zix_free(NULL, tmpdir);

  return result;
}
