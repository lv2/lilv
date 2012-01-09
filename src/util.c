/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

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

#define _POSIX_C_SOURCE 1  /* for wordexp, fileno */
#define _BSD_SOURCE     1  /* for realpath, symlink */

#ifdef __APPLE__
#    define _DARWIN_C_SOURCE 1  /* for flock */
#endif

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "lilv_internal.h"

#if defined(HAVE_FLOCK) && defined(HAVE_FILENO)
#    include <sys/file.h>
#endif

#ifdef HAVE_WORDEXP
#    include <wordexp.h>
#endif

char*
lilv_strjoin(const char* first, ...)
{
	size_t  len    = strlen(first);
	char*   result = malloc(len + 1);

	memcpy(result, first, len);

	va_list args;
	va_start(args, first);
	while (1) {
		const char* const s = va_arg(args, const char *);
		if (s == NULL)
			break;

		const size_t this_len = strlen(s);
		result = realloc(result, len + this_len + 1);
		memcpy(result + len, s, this_len);
		len += this_len;
	}
	va_end(args);

	result[len] = '\0';

	return result;
}

char*
lilv_strdup(const char* str)
{
	const size_t len = strlen(str);
	char*        dup = malloc(len + 1);
	memcpy(dup, str, len + 1);
	return dup;
}

const char*
lilv_uri_to_path(const char* uri)
{
	return (const char*)serd_uri_to_path((const uint8_t*)uri);
}

/** Return the current LANG converted to Turtle (i.e. RFC3066) style.
 * For example, if LANG is set to "en_CA.utf-8", this returns "en-ca".
 */
char*
lilv_get_lang(void)
{
	const char* const env_lang = getenv("LANG");
	if (!env_lang || !strcmp(env_lang, "")
	    || !strcmp(env_lang, "C") || !strcmp(env_lang, "POSIX")) {
		return NULL;
	}

	const size_t env_lang_len = strlen(env_lang);
	char* const  lang         = malloc(env_lang_len + 1);
	for (size_t i = 0; i < env_lang_len + 1; ++i) {
		if (env_lang[i] == '_') {
			lang[i] = '-';  // Convert _ to -
		} else if (env_lang[i] >= 'A' && env_lang[i] <= 'Z') {
			lang[i] = env_lang[i] + ('a' - 'A');  // Convert to lowercase
		} else if (env_lang[i] >= 'a' && env_lang[i] <= 'z') {
			lang[i] = env_lang[i];  // Lowercase letter, copy verbatim
		} else if (env_lang[i] >= '0' && env_lang[i] <= '9') {
			lang[i] = env_lang[i];  // Digit, copy verbatim
		} else if (env_lang[i] == '\0' || env_lang[i] == '.') {
			// End, or start of suffix (e.g. en_CA.utf-8), finished
			lang[i] = '\0';
			break;
		} else {
			LILV_ERRORF("Illegal LANG `%s' ignored\n", env_lang);
			free(lang);
			return NULL;
		}
	}

	return lang;
}

/** Expand variables (e.g. POSIX ~ or $FOO, Windows %FOO%) in @a path. */
char*
lilv_expand(const char* path)
{
#ifdef HAVE_WORDEXP
	char*     ret = NULL;
	wordexp_t p;
	if (wordexp(path, &p, 0)) {
		LILV_ERRORF("Error expanding path `%s'\n", path);
		return lilv_strdup(path);
	}
	if (p.we_wordc == 0) {
		/* Literal directory path (e.g. no variables or ~) */
		ret = lilv_strdup(path);
	} else if (p.we_wordc == 1) {
		/* Directory path expands (e.g. contains ~ or $FOO) */
		ret = lilv_strdup(p.we_wordv[0]);
	} else {
		/* Multiple expansions in a single directory path? */
		LILV_ERRORF("Malformed path `%s'\n", path);
		ret = lilv_strdup(path);
	}
	wordfree(&p);
#elif defined(__WIN32__)
	static const size_t len = 32767;
	char*               ret = malloc(len);
	ExpandEnvironmentStrings(path, ret, len);
#else
	char* ret = lilv_strdup(path);
#endif
	return ret;
}

char*
lilv_dirname(const char* path)
{
	const char* s = path + strlen(path) - 1;  // Last character
	for (; s > path && *s == LILV_DIR_SEP[0]; --s) {}  // Last non-slash
	for (; s > path && *s != LILV_DIR_SEP[0]; --s) {}  // Last internal slash
	for (; s > path && *s == LILV_DIR_SEP[0]; --s) {}  // Skip duplicates

	if (s == path) {  // Hit beginning
		return (*s == '/') ? lilv_strdup("/") : lilv_strdup(".");
	} else {  // Pointing to the last character of the result (inclusive)
		char* dirname = malloc(s - path + 2);
		memcpy(dirname, path, s - path + 1);
		dirname[s - path + 1] = '\0';
		return dirname;
	}
}

bool
lilv_path_exists(const char* path, void* ignored)
{
	return !access(path, F_OK);
}

char*
lilv_find_free_path(
	const char* in_path, bool (*exists)(const char*, void*), void* user_data)
{
	const size_t in_path_len = strlen(in_path);
	char*        path        = malloc(in_path_len + 7);
	memcpy(path, in_path, in_path_len + 1);

	for (int i = 2; i < 1000000; ++i) {
		if (!exists(path, user_data)) {
			return path;
		}
		snprintf(path, in_path_len + 7, "%s%u", in_path, i);
	}

	return NULL;
}

int
lilv_copy_file(const char* src, const char* dst)
{
	FILE* in = fopen(src, "r");
	if (!in) {
		LILV_ERRORF("error opening %s (%s)\n", src, strerror(errno));
		return 1;
	}

	FILE* out = fopen(dst, "w");
	if (!out) {
		LILV_ERRORF("error opening %s (%s)\n", dst, strerror(errno));
		fclose(in);
		return 2;
	}

	static const size_t PAGE_SIZE = 4096;
	char*               page      = malloc(PAGE_SIZE);
	size_t              n_read    = 0;
	while ((n_read = fread(page, 1, PAGE_SIZE, in)) > 0) {
		if (fwrite(page, 1, n_read, out) != n_read) {
			LILV_ERRORF("write to %s failed (%s)\n", dst, strerror(errno));
			break;
		}
	}

	const int ret = ferror(in) || ferror(out);
	if (ferror(in)) {
		LILV_ERRORF("read from %s failed (%s)\n", src, strerror(errno));
	}

	free(page);
	fclose(in);
	fclose(out);

	return ret;
}

static bool
lilv_is_dir_sep(const char c)
{
	return c == '/' || c == LILV_DIR_SEP[0];
}

bool
lilv_path_is_absolute(const char* path)
{
	if (lilv_is_dir_sep(path[0])) {
		return true;
	}

#ifdef __WIN32__
	if (isalpha(path[0]) && path[1] == ':' && lilv_is_dir_sep(path[2])) {
		return true;
	}
#endif

	return false;
}

char*
lilv_path_join(const char* a, const char* b)
{
	const size_t a_len   = strlen(a);
	const size_t b_len   = strlen(b);
	const size_t pre_len = a_len - (lilv_is_dir_sep(a[a_len - 1]) ? 1 : 0);
	char*        path    = calloc(1, a_len + b_len + 2);
	memcpy(path, a, pre_len);
	path[pre_len] = LILV_DIR_SEP[0];
	memcpy(path + pre_len + 1,
	       b + (lilv_is_dir_sep(b[0]) ? 1 : 0),
	       lilv_is_dir_sep(b[0]) ? b_len - 1 : b_len);
	return path;
}

static void
lilv_size_mtime(const char* path, off_t* size, time_t* time)
{
	struct stat buf;
	if (stat(path, &buf)) {
		LILV_ERRORF("stat(%s) (%s)\n", path, strerror(errno));
		*size = *time = 0;
	}

	*size = buf.st_size;
	*time = buf.st_mtime;
}

typedef struct {
	char*  pattern;
	off_t  orig_size;
	time_t time;
	char*  latest;
} Latest;

static void
update_latest(const char* path, const char* name, void* data)
{
	Latest* latest     = (Latest*)data;
	char*   entry_path = lilv_strjoin(path, "/", name, NULL);
	unsigned num;
	if (sscanf(entry_path, latest->pattern, &num) == 1) {
		off_t  entry_size;
		time_t entry_time;
		lilv_size_mtime(entry_path, &entry_size, &entry_time);
		if (entry_size == latest->orig_size && entry_time >= latest->time) {
			free(latest->latest);
			latest->latest = entry_path;
		}
	}
	if (entry_path != latest->latest) {
		free(entry_path);
	}
}	
	              
/** Return the latest copy of the file at @c path that is newer. */
char*
lilv_get_latest_copy(const char* path)
{
	char*  dirname = lilv_dirname(path);
	Latest latest  = { lilv_strjoin(path, "%u", NULL), 0, 0, NULL };
	lilv_size_mtime(path, &latest.orig_size, &latest.time);

	lilv_dir_for_each(dirname, &latest, update_latest);

	free(latest.pattern);
	return latest.latest;
}

char*
lilv_realpath(const char* path)
{
	return realpath(path, NULL);
}

int
lilv_symlink(const char* oldpath, const char* newpath)
{
	return symlink(oldpath, newpath);
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
	char*        rel        = calloc(1, suffix_len + (up * 3) + 1);
	for (size_t i = 0; i < up; ++i) {
		memcpy(rel + (i * 3), ".." LILV_DIR_SEP, 3);
	}

	// Write suffix
	memcpy(rel + (up * 3), path + last_shared_sep + 1, suffix_len);
	return rel;
}

bool
lilv_path_is_child(const char* path, const char* dir)
{
	const size_t path_len = strlen(path);
	const size_t dir_len  = strlen(dir);
	return dir && path_len >= dir_len && !strncmp(path, dir, dir_len);
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
	DIR* dir = opendir(path);
	if (dir) {
		struct dirent  entry;
		struct dirent* result;
		while (!readdir_r(dir, &entry, &result) && result) {
			f(path, entry.d_name, data);
		}
		closedir(dir);
	}
}

int
lilv_mkdir_p(const char* dir_path)
{
	char*        path     = lilv_strdup(dir_path);
	const size_t path_len = strlen(path);
	for (size_t i = 1; i <= path_len; ++i) {
		if (path[i] == LILV_DIR_SEP[0] || path[i] == '\0') {
			path[i] = '\0';
			if (mkdir(path, 0755) && errno != EEXIST) {
				LILV_ERRORF("Failed to create %s (%s)\n",
				            path, strerror(errno));
				free(path);
				return 1;
			}
			path[i] = LILV_DIR_SEP[0];
		}
	}

	free(path);
	return 0;
}
