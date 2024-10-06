// Copyright 2007-2024 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "../src/lilv_internal.h"

#ifdef _WIN32
#  include <windows.h>
#  define setenv(n, v, r) SetEnvironmentVariable((n), (v))
#  define unsetenv(n) SetEnvironmentVariable((n), NULL)
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
static void
check_expansion(const char* const path, const char* const expected)
{
  char* const expanded = lilv_expand(path);
  assert(!strcmp(expanded, expected));
  free(expanded);
}
#endif

int
main(void)
{
#ifndef _WIN32
  setenv("LILV_TEST_1", "test", 1);

  const char* const home = getenv("HOME");

  check_expansion("$LILV_TEST_1", "test");

  if (home) {
    char* const home_foo = lilv_strjoin(home, "/foo", NULL);
    check_expansion("~", home);
    check_expansion("~foo", "~foo");
    check_expansion("~/foo", home_foo);
    free(home_foo);
  }

  check_expansion("$NOT_A_VAR", "$NOT_A_VAR");

  unsetenv("LILV_TEST_1");
#endif

  return 0;
}
