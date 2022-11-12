// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "lilv_internal.h"

#include "../src/filesystem.h"

#include "zix/filesystem.h"
#include "zix/path.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
equals(char* string, const char* expected)
{
  const bool result = !strcmp(string, expected);
  free(string);
  return result;
}

static void
test_path_is_child(void)
{
  assert(lilv_path_is_child("/a/b", "/a"));
  assert(lilv_path_is_child("/a/b", "/a/"));
  assert(lilv_path_is_child("/a/b/", "/a"));
  assert(lilv_path_is_child("/a/b/", "/a/"));

  assert(!lilv_path_is_child("/a/b", "/a/c"));
  assert(!lilv_path_is_child("/a/b", "/a/c/"));
  assert(!lilv_path_is_child("/a/b/", "/a/c"));
  assert(!lilv_path_is_child("/a/b/", "/a/c/"));

  assert(!lilv_path_is_child("/a/b", "/c"));
  assert(!lilv_path_is_child("/a/b", "/c/"));
  assert(!lilv_path_is_child("/a/b/", "/c"));
  assert(!lilv_path_is_child("/a/b/", "/c/"));
}

static void
test_path_current(void)
{
  char* cwd = lilv_path_current();

  assert(lilv_is_directory(cwd));

  free(cwd);
}

static void
test_path_relative_to(void)
{
  assert(equals(lilv_path_relative_to("/a/b", "/a/"), "b"));
  assert(equals(lilv_path_relative_to("/a", "/b/c/"), "/a"));
  assert(equals(lilv_path_relative_to("/a/b/c", "/a/b/d/"), "../c"));
  assert(equals(lilv_path_relative_to("/a/b/c", "/a/b/d/e/"), "../../c"));

#ifdef _WIN32
  assert(equals(lilv_path_relative_to("C:/a/b", "C:/a/"), "b"));
  assert(equals(lilv_path_relative_to("C:/a", "C:/b/c/"), "../../a"));
  assert(equals(lilv_path_relative_to("C:/a/b/c", "C:/a/b/d/"), "../c"));
  assert(equals(lilv_path_relative_to("C:/a/b/c", "C:/a/b/d/e/"), "../../c"));

  assert(equals(lilv_path_relative_to("C:\\a\\b", "C:\\a\\"), "b"));
  assert(equals(lilv_path_relative_to("C:\\a", "C:\\b\\c\\"), "..\\..\\a"));
  assert(
    equals(lilv_path_relative_to("C:\\a\\b\\c", "C:\\a\\b\\d\\"), "..\\c"));
  assert(equals(lilv_path_relative_to("C:\\a\\b\\c", "C:\\a\\b\\d\\e\\"),
                "..\\..\\c"));
#endif
}

static void
test_path_parent(void)
{
  assert(equals(lilv_path_parent("/"), "/"));
  assert(equals(lilv_path_parent("//"), "/"));
  assert(equals(lilv_path_parent("/a"), "/"));
  assert(equals(lilv_path_parent("/a/"), "/"));
  assert(equals(lilv_path_parent("/a///b/"), "/a"));
  assert(equals(lilv_path_parent("/a///b//"), "/a"));
  assert(equals(lilv_path_parent("/a/b"), "/a"));
  assert(equals(lilv_path_parent("/a/b/"), "/a"));
  assert(equals(lilv_path_parent("/a/b/c"), "/a/b"));
  assert(equals(lilv_path_parent("/a/b/c/"), "/a/b"));
  assert(equals(lilv_path_parent("a"), "."));
}

static void
test_path_filename(void)
{
  // Cases from cppreference.com for std::filesystem::path::filename
  assert(equals(lilv_path_filename("/foo/bar.txt"), "bar.txt"));
  assert(equals(lilv_path_filename("/foo/.bar"), ".bar"));
  assert(equals(lilv_path_filename("/foo/bar/"), ""));
  assert(equals(lilv_path_filename("/foo/."), "."));
  assert(equals(lilv_path_filename("/foo/.."), ".."));
  assert(equals(lilv_path_filename("."), "."));
  assert(equals(lilv_path_filename(".."), ".."));
  assert(equals(lilv_path_filename("/"), ""));
  assert(equals(lilv_path_filename("//host"), "host"));

#ifdef _WIN32
  assert(equals(lilv_path_filename("C:/foo/bar.txt"), "bar.txt"));
  assert(equals(lilv_path_filename("C:\\foo\\bar.txt"), "bar.txt"));
  assert(equals(lilv_path_filename("foo/bar.txt"), "bar.txt"));
  assert(equals(lilv_path_filename("foo\\bar.txt"), "bar.txt"));
#endif
}

static void
test_is_directory(void)
{
  char* const temp_dir  = lilv_create_temporary_directory("lilvXXXXXX");
  char* const file_path = zix_path_join(NULL, temp_dir, "lilv_test_file");

  assert(lilv_is_directory(temp_dir));
  assert(!lilv_is_directory(file_path)); // Nonexistent

  FILE* f = fopen(file_path, "w");
  fprintf(f, "test\n");
  fclose(f);

  assert(!lilv_is_directory(file_path)); // File

  assert(!zix_remove(file_path));
  assert(!zix_remove(temp_dir));

  free(file_path);
  free(temp_dir);
}

typedef struct {
  size_t n_names;
  char** names;
} FileList;

static void
visit(const char* const path, const char* const name, void* const data)
{
  (void)path;

  FileList* file_list = (FileList*)data;

  file_list->names =
    (char**)realloc(file_list->names, sizeof(char*) * ++file_list->n_names);

  file_list->names[file_list->n_names - 1] = lilv_strdup(name);
}

static void
test_dir_for_each(void)
{
  char* const temp_dir = lilv_create_temporary_directory("lilvXXXXXX");
  char* const path1    = zix_path_join(NULL, temp_dir, "lilv_test_1");
  char* const path2    = zix_path_join(NULL, temp_dir, "lilv_test_2");

  FILE* const f1 = fopen(path1, "w");
  FILE* const f2 = fopen(path2, "w");
  fprintf(f1, "test\n");
  fprintf(f2, "test\n");
  fclose(f2);
  fclose(f1);

  FileList file_list = {0, NULL};
  lilv_dir_for_each(temp_dir, &file_list, visit);

  assert((!strcmp(file_list.names[0], "lilv_test_1") &&
          !strcmp(file_list.names[1], "lilv_test_2")) ||
         (!strcmp(file_list.names[0], "lilv_test_2") &&
          !strcmp(file_list.names[1], "lilv_test_1")));

  assert(!zix_remove(path2));
  assert(!zix_remove(path1));
  assert(!zix_remove(temp_dir));

  free(file_list.names[0]);
  free(file_list.names[1]);
  free(file_list.names);
  free(path2);
  free(path1);
  free(temp_dir);
}

int
main(void)
{
  test_path_is_child();
  test_path_current();
  test_path_relative_to();
  test_path_parent();
  test_path_filename();
  test_is_directory();
  test_dir_for_each();

  return 0;
}
