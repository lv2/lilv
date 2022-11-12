// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "../src/filesystem.h"

#include <assert.h>

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

int
main(void)
{
  test_path_is_child();

  return 0;
}
