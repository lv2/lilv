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

int
main(void)
{
  test_path_is_child();
  test_path_parent();

  return 0;
}
