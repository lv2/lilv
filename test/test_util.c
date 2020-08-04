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

#define _XOPEN_SOURCE 600 /* for mkstemp */

#undef NDEBUG

#ifdef _WIN32
#	 include "lilv_internal.h"
#endif

#include "../src/filesystem.h"

#ifdef _WIN32
#    include <io.h>
#    define mkstemp(pat) _mktemp(pat)
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(void)
{
	assert(!lilv_path_canonical(NULL));

	char a_path[16];
	char b_path[16];
	strncpy(a_path, "copy_a_XXXXXX", sizeof(a_path));
	strncpy(b_path, "copy_b_XXXXXX", sizeof(b_path));
	mkstemp(a_path);
	mkstemp(b_path);

	FILE* fa = fopen(a_path, "w");
	FILE* fb = fopen(b_path, "w");
	fprintf(fa, "AA\n");
	fprintf(fb, "AB\n");
	fclose(fa);
	fclose(fb);

	assert(lilv_copy_file("does/not/exist", "copy"));
	assert(lilv_copy_file(a_path, "not/a/dir/copy"));
	assert(!lilv_copy_file(a_path, "copy_c"));
	assert(!lilv_file_equals(a_path, b_path));
	assert(lilv_file_equals(a_path, a_path));
	assert(lilv_file_equals(a_path, "copy_c"));
	assert(!lilv_file_equals("does/not/exist", b_path));
	assert(!lilv_file_equals(a_path, "does/not/exist"));
	assert(!lilv_file_equals("does/not/exist", "/does/not/either"));

	return 0;
}
