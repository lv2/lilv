/*
  Copyright 2007-2022 David Robillard <d@drobilla.net>

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

#undef NDEBUG

#include "../src/filesystem.h"

#include "lilv/lilv.h"

#include <assert.h>
#include <stdio.h>

int
main(void)
{
  assert(!lilv_path_canonical(NULL));

  char* const dir    = lilv_create_temporary_directory("lilv_test_util_XXXXXX");
  char* const a_path = lilv_path_join(dir, "copy_a_XXXXXX");
  char* const b_path = lilv_path_join(dir, "copy_b_XXXXXX");

  FILE* const fa = fopen(a_path, "w");
  FILE* const fb = fopen(b_path, "w");
  fprintf(fa, "AA\n");
  fprintf(fb, "AB\n");
  fclose(fb);
  fclose(fa);

  assert(lilv_copy_file("does/not/exist", "copy"));
  assert(lilv_copy_file(a_path, "not/a/dir/copy"));
  assert(!lilv_copy_file(a_path, "copy_c"));
  assert(!lilv_file_equals(a_path, b_path));
  assert(lilv_file_equals(a_path, a_path));
  assert(lilv_file_equals(a_path, "copy_c"));
  assert(!lilv_file_equals("does/not/exist", b_path));
  assert(!lilv_file_equals(a_path, "does/not/exist"));
  assert(!lilv_file_equals("does/not/exist", "/does/not/either"));

  assert(!lilv_remove(a_path));
  assert(!lilv_remove(b_path));
  assert(!lilv_remove(dir));

  lilv_free(b_path);
  lilv_free(a_path);
  lilv_free(dir);
  return 0;
}
