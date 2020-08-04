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

#define _POSIX_C_SOURCE 200809L  /* for fileno */
#define _BSD_SOURCE     1        /* for realpath, symlink */
#define _DEFAULT_SOURCE 1        /* for realpath, symlink */

#ifdef __APPLE__
#    define _DARWIN_C_SOURCE 1  /* for flock */
#endif

#include "filesystem.h"
#include "lilv_config.h"
#include "lilv_internal.h"

#ifdef _WIN32
#    include <windows.h>
#    include <direct.h>
#    include <io.h>
#    define F_OK 0
#    define mkdir(path, flags) _mkdir(path)
#else
#    include <dirent.h>
#    include <unistd.h>
#endif

#if defined(HAVE_FLOCK) && defined(HAVE_FILENO)
#    include <sys/file.h>
#endif

#include <sys/stat.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PAGE_SIZE
#    define PAGE_SIZE 4096
#endif

static bool
lilv_is_dir_sep(const char c)
{
	return c == '/' || c == LILV_DIR_SEP[0];
}

#ifdef _WIN32
static inline bool
is_windows_path(const char* path)
{
	return (isalpha(path[0]) && (path[1] == ':' || path[1] == '|') &&
	        (path[2] == '/' || path[2] == '\\'));
}
#endif

bool
lilv_path_is_absolute(const char* path)
{
	if (lilv_is_dir_sep(path[0])) {
		return true;
	}

#ifdef _WIN32
	if (is_windows_path(path)) {
		return true;
	}
#endif

	return false;
}

bool
lilv_path_is_child(const char* path, const char* dir)
{
	if (path && dir) {
		const size_t path_len = strlen(path);
		const size_t dir_len  = strlen(dir);
		return dir && path_len >= dir_len && !strncmp(path, dir, dir_len);
	}
	return false;
}

char*
lilv_path_absolute(const char* path)
{
	if (lilv_path_is_absolute(path)) {
		return lilv_strdup(path);
	} else {
		char* cwd      = getcwd(NULL, 0);
		char* abs_path = lilv_path_join(cwd, path);
		free(cwd);
		return abs_path;
	}
}

char*
lilv_path_relative_to(const char* path, const char* base)
{
	const size_t path_len = strlen(path);
	const size_t base_len = strlen(base);
	const size_t min_len  = (path_len < base_len) ? path_len : base_len;

	// Find the last separator common to both paths
	size_t last_shared_sep = 0;
	for (size_t i = 0; i < min_len && path[i] == base[i]; ++i) {
		if (lilv_is_dir_sep(path[i])) {
			last_shared_sep = i;
		}
	}

	if (last_shared_sep == 0) {
		// No common components, return path
		return lilv_strdup(path);
	}

	// Find the number of up references ("..") required
	size_t up = 0;
	for (size_t i = last_shared_sep + 1; i < base_len; ++i) {
		if (lilv_is_dir_sep(base[i])) {
			++up;
		}
	}

	// Write up references
	const size_t suffix_len = path_len - last_shared_sep;
	char*        rel        = (char*)calloc(1, suffix_len + (up * 3) + 1);
	for (size_t i = 0; i < up; ++i) {
		memcpy(rel + (i * 3), "../", 3);
	}

	// Write suffix
	memcpy(rel + (up * 3), path + last_shared_sep + 1, suffix_len);
	return rel;
}

char*
lilv_path_parent(const char* path)
{
	const char* s = path + strlen(path) - 1;  // Last character
	for (; s > path && lilv_is_dir_sep(*s); --s) {}  // Last non-slash
	for (; s > path && !lilv_is_dir_sep(*s); --s) {}  // Last internal slash
	for (; s > path && lilv_is_dir_sep(*s); --s) {}  // Skip duplicates

	if (s == path) {  // Hit beginning
		return lilv_is_dir_sep(*s) ? lilv_strdup("/") : lilv_strdup(".");
	} else {  // Pointing to the last character of the result (inclusive)
		char* dirname = (char*)malloc(s - path + 2);
		memcpy(dirname, path, s - path + 1);
		dirname[s - path + 1] = '\0';
		return dirname;
	}
}

char*
lilv_path_join(const char* a, const char* b)
{
	if (!a) {
		return lilv_strdup(b);
	}

	const size_t a_len   = strlen(a);
	const size_t b_len   = b ? strlen(b) : 0;
	const size_t pre_len = a_len - (lilv_is_dir_sep(a[a_len - 1]) ? 1 : 0);
	char*        path    = (char*)calloc(1, a_len + b_len + 2);
	memcpy(path, a, pre_len);
	path[pre_len] = '/';
	if (b) {
		memcpy(path + pre_len + 1,
		       b + (lilv_is_dir_sep(b[0]) ? 1 : 0),
		       lilv_is_dir_sep(b[0]) ? b_len - 1 : b_len);
	}
	return path;
}

char*
lilv_dir_path(const char* path)
{
	if (!path) {
		return NULL;
	}

	const size_t len = strlen(path);

	if (lilv_is_dir_sep(path[len - 1])) {
		return lilv_strdup(path);
	}

	char* dir_path = (char*)calloc(len + 2, 1);
	memcpy(dir_path, path, len);
	dir_path[len] = LILV_DIR_SEP[0];
	return dir_path;
}

char*
lilv_path_canonical(const char* path)
{
	if (!path) {
		return NULL;
	}

#if defined(_WIN32)
	char* out = (char*)malloc(MAX_PATH);
	GetFullPathName(path, MAX_PATH, out, NULL);
	return out;
#else
	char* real_path = realpath(path, NULL);
	return real_path ? real_path : lilv_strdup(path);
#endif
}

bool
lilv_path_exists(const char* path)
{
#ifdef HAVE_LSTAT
	struct stat st;
	return !lstat(path, &st);
#else
	return !access(path, F_OK);
#endif
}

int
lilv_copy_file(const char* src, const char* dst)
{
	FILE* in = fopen(src, "r");
	if (!in) {
		return errno;
	}

	FILE* out = fopen(dst, "w");
	if (!out) {
		fclose(in);
		return errno;
	}

	char*  page   = (char*)malloc(PAGE_SIZE);
	size_t n_read = 0;
	int    st     = 0;
	while ((n_read = fread(page, 1, PAGE_SIZE, in)) > 0) {
		if (fwrite(page, 1, n_read, out) != n_read) {
			st = errno;
			break;
		}
	}

	if (!st && (ferror(in) || ferror(out))) {
		st = EBADF;
	}

	free(page);
	fclose(in);
	fclose(out);

	return st;
}

int
lilv_symlink(const char* oldpath, const char* newpath)
{
	int ret = 0;
	if (strcmp(oldpath, newpath)) {
#ifdef _WIN32
#ifdef HAVE_CREATESYMBOLICLINK
		ret = !CreateSymbolicLink(newpath, oldpath, 0);
#endif
		if (ret) {
			ret = !CreateHardLink(newpath, oldpath, 0);
		}
#else
		ret = symlink(oldpath, newpath);
#endif
	}
	return ret;
}

int
lilv_flock(FILE* file, bool lock)
{
#if defined(HAVE_FLOCK) && defined(HAVE_FILENO)
	return flock(fileno(file), lock ? LOCK_EX : LOCK_UN);
#else
	return 0;
#endif
}

void
lilv_dir_for_each(const char* path,
                  void*       data,
                  void (*f)(const char* path, const char* name, void* data))
{
#ifdef _WIN32
	char*           pat = lilv_path_join(path, "*");
	WIN32_FIND_DATA fd;
	HANDLE          fh  = FindFirstFile(pat, &fd);
	if (fh != INVALID_HANDLE_VALUE) {
		do {
			f(path, fd.cFileName, data);
		} while (FindNextFile(fh, &fd));
	}
	free(pat);
#else
	DIR* dir = opendir(path);
	if (dir) {
		for (struct dirent* entry = NULL; (entry = readdir(dir));) {
			f(path, entry->d_name, data);
		}
		closedir(dir);
	}
#endif
}

int
lilv_create_directories(const char* dir_path)
{
	char*        path     = lilv_strdup(dir_path);
	const size_t path_len = strlen(path);
	size_t       i        = 1;

#ifdef _WIN32
	if (is_windows_path(dir_path)) {
		i = 3;
	}
#endif

	for (; i <= path_len; ++i) {
		const char c = path[i];
		if (c == LILV_DIR_SEP[0] || c == '/' || c == '\0') {
			path[i] = '\0';
			if (mkdir(path, 0755) && errno != EEXIST) {
				free(path);
				return errno;
			}
			path[i] = c;
		}
	}

	free(path);
	return 0;
}

static off_t
lilv_file_size(const char* path)
{
	struct stat buf;
	if (stat(path, &buf)) {
		return 0;
	}
	return buf.st_size;
}

bool
lilv_file_equals(const char* a_path, const char* b_path)
{
	if (!strcmp(a_path, b_path)) {
		return true;  // Paths match
	}

	bool        match  = false;
	FILE*       a_file = NULL;
	FILE*       b_file = NULL;
	char* const a_real = lilv_path_canonical(a_path);
	char* const b_real = lilv_path_canonical(b_path);
	if (!strcmp(a_real, b_real)) {
		match = true;  // Real paths match
	} else if (lilv_file_size(a_path) != lilv_file_size(b_path)) {
		match = false;  // Sizes differ
	} else if (!(a_file = fopen(a_real, "rb")) ||
	           !(b_file = fopen(b_real, "rb"))) {
		match = false;  // Missing file matches nothing
	} else {
		// TODO: Improve performance by reading chunks
		match = true;
		while (!feof(a_file) && !feof(b_file)) {
			if (fgetc(a_file) != fgetc(b_file)) {
				match = false;
				break;
			}
		}
	}

	if (a_file) {
		fclose(a_file);
	}
	if (b_file) {
		fclose(b_file);
	}
	free(a_real);
	free(b_real);
	return match;
}
