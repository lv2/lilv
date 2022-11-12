// Copyright 2007-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <stdbool.h>

/// Return true iff `path` is a child of `dir`
bool
lilv_path_is_child(const char* path, const char* dir);

/**
   Create a unique temporary directory.

   This is like lilv_create_temporary_directory_in(), except it creates the
   directory in the system temporary directory.
*/
char*
lilv_create_temporary_directory(const char* pattern);
