/*
  Copyright 2007-2020 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define _POSIX_C_SOURCE 200809L /* for setenv */

#undef NDEBUG

#include "../src/lilv_internal.h"

#ifdef _WIN32
#	include <windows.h>
#	define setenv(n, v, r) SetEnvironmentVariable((n), (v))
#	define unsetenv(n) SetEnvironmentVariable((n), NULL)
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int
main(void)
{
#ifndef _WIN32
	char* s = NULL;

	setenv("LILV_TEST_1", "test", 1);
	char* home_foo = lilv_strjoin(getenv("HOME"), "/foo", NULL);
	assert(!strcmp((s = lilv_expand("$LILV_TEST_1")), "test"));
	free(s);
	assert(!strcmp((s = lilv_expand("~")), getenv("HOME")));
	free(s);
	assert(!strcmp((s = lilv_expand("~foo")), "~foo"));
	free(s);
	assert(!strcmp((s = lilv_expand("~/foo")), home_foo));
	free(s);
	assert(!strcmp((s = lilv_expand("$NOT_A_VAR")), "$NOT_A_VAR"));
	free(s);
	free(home_foo);
	unsetenv("LILV_TEST_1");
#endif

	return 0;
}
