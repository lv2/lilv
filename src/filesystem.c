// Copyright 2007-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "filesystem.h"

#include "zix/allocator.h"
#include "zix/filesystem.h"
#include "zix/path.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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
lilv_create_temporary_directory(const char* pattern)
{
  char* const tmpdir       = zix_temp_directory_path(NULL);
  char* const path_pattern = zix_path_join(NULL, tmpdir, pattern);
  char* const result       = zix_create_temporary_directory(NULL, path_pattern);

  zix_free(NULL, path_pattern);
  zix_free(NULL, tmpdir);

  return result;
}
