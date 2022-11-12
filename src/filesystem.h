// Copyright 2007-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <stdbool.h>

/// Return true iff `path` is a child of `dir`
bool
lilv_path_is_child(const char* path, const char* dir);

/// Return the current working directory
char*
lilv_path_current(void);

/**
   Return `path` relative to `base` if possible.

   If `path` is not within `base`, a copy is returned.  Otherwise, an
   equivalent path relative to `base` is returned (which may contain
   up-references).
*/
char*
lilv_path_relative_to(const char* path, const char* base);

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

/// Return true iff `path` points to an existing directory
bool
lilv_is_directory(const char* path);

/**
   Visit every file in the directory at `path`.

   @param path A path to a directory.

   @param data Opaque user data that is passed to `f`.

   @param f A function called on every entry in the directory.  The `path`
   parameter is always the directory path passed to this function, the `name`
   parameter is the name of the directory entry (not its full path).
*/
void
lilv_dir_for_each(const char* path,
                  void*       data,
                  void (*f)(const char* path, const char* name, void* data));

/**
   Create a unique temporary directory.

   This is like lilv_create_temporary_directory_in(), except it creates the
   directory in the system temporary directory.
*/
char*
lilv_create_temporary_directory(const char* pattern);
