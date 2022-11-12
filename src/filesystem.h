// Copyright 2007-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <stdbool.h>

/// Return true iff `path` is a child of `dir`
bool
lilv_path_is_child(const char* path, const char* dir);

/**
   Return the path to the directory that contains `path`.

   Returns the root path if `path` is the root path.
*/
char*
lilv_path_parent(const char* path);

/**
   Return the filename component of `path` without any directories.

   Returns the empty string if `path` is the root path.
*/
char*
lilv_path_filename(const char* path);

/**
   Create a unique temporary directory.

   This is like lilv_create_temporary_directory_in(), except it creates the
   directory in the system temporary directory.
*/
char*
lilv_create_temporary_directory(const char* pattern);
