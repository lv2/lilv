// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "../src/filesystem.h"

#include <assert.h>
#include <stdbool.h>
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

int
main(void)
{
  test_path_is_child();
  test_path_relative_to();
  test_path_parent();
  test_path_filename();

  return 0;
}
