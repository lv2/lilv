// Copyright 2007-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "filesystem.h"
#include "lilv_config.h"
#include "lilv_internal.h"

#include "zix/allocator.h"
#include "zix/filesystem.h"
#include "zix/path.h"

#ifdef _WIN32
#  include <direct.h>
#  include <io.h>
#  include <windows.h>
#  define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#else
#  include <dirent.h>
#  include <unistd.h>
#endif

#include <sys/stat.h>

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
lilv_path_current(void)
{
  return getcwd(NULL, 0);
}

char*
lilv_path_relative_to(const char* path, const char* base)
{
  const size_t path_len = strlen(path);
  const size_t base_len = strlen(base);
  const size_t min_len  = (path_len < base_len) ? path_len : base_len;

  // Find the last separator common to both paths
  size_t last_shared_sep = 0;
  for (size_t i = 0; i < min_len && path[i] == base[i]; ++i) {
    if (lilv_is_dir_sep(path[i])) {
      last_shared_sep = i;
    }
  }

  if (last_shared_sep == 0) {
    // No common components, return path
    return lilv_strdup(path);
  }

  // Find the number of up references ("..") required
  size_t up = 0;
  for (size_t i = last_shared_sep + 1; i < base_len; ++i) {
    if (lilv_is_dir_sep(base[i])) {
      ++up;
    }
  }

#ifdef _WIN32
  const bool use_slash = strchr(path, '/');
#else
  static const bool use_slash = true;
#endif

  // Write up references
  const size_t suffix_len = path_len - last_shared_sep;
  char*        rel        = (char*)calloc(1, suffix_len + (up * 3) + 1);
  for (size_t i = 0; i < up; ++i) {
    if (use_slash) {
      memcpy(rel + (i * 3), "../", 3);
    } else {
      memcpy(rel + (i * 3), "..\\", 3);
    }
  }

  // Write suffix
  memcpy(rel + (up * 3), path + last_shared_sep + 1, suffix_len);
  return rel;
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
lilv_path_filename(const char* path)
{
  const size_t path_len = strlen(path);
  size_t       last_sep = path_len;
  for (size_t i = 0; i < path_len; ++i) {
    if (lilv_is_dir_sep(path[i])) {
      last_sep = i;
    }
  }

  if (last_sep >= path_len) {
    return lilv_strdup(path);
  }

  const size_t ret_len = path_len - last_sep;
  char* const  ret     = (char*)calloc(ret_len + 1, 1);

  strncpy(ret, path + last_sep + 1, ret_len);
  return ret;
}

bool
lilv_is_directory(const char* path)
{
#if defined(_WIN32)
  const DWORD attrs = GetFileAttributes(path);

  return (attrs != INVALID_FILE_ATTRIBUTES) &&
         (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
  struct stat       st;
  return !stat(path, &st) && S_ISDIR(st.st_mode);
#endif
}

int
lilv_symlink(const char* oldpath, const char* newpath)
{
  int ret = 0;
  if (strcmp(oldpath, newpath)) {
#ifdef _WIN32
    ret = !CreateHardLink(newpath, oldpath, 0);
#else
    char* target = lilv_path_relative_to(oldpath, newpath);

    ret = symlink(target, newpath);

    free(target);
#endif
  }
  return ret;
}

void
lilv_dir_for_each(const char* path,
                  void*       data,
                  void (*f)(const char* path, const char* name, void* data))
{
#ifdef _WIN32
  char*           pat = zix_path_join(NULL, path, "*");
  WIN32_FIND_DATA fd;
  HANDLE          fh = FindFirstFile(pat, &fd);
  if (fh != INVALID_HANDLE_VALUE) {
    do {
      if (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..")) {
        f(path, fd.cFileName, data);
      }
    } while (FindNextFile(fh, &fd));
  }
  FindClose(fh);
  free(pat);
#else
  DIR* dir = opendir(path);
  if (dir) {
    for (struct dirent* entry = NULL; (entry = readdir(dir));) {
      if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
        f(path, entry->d_name, data);
      }
    }
    closedir(dir);
  }
#endif
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
