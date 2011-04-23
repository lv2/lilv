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

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "slv2_internal.h"

char*
slv2_strjoin(const char* first, ...)
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
slv2_strdup(const char* str)
{
	const size_t len = strlen(str);
	char*        dup = malloc(len + 1);
	memcpy(dup, str, len + 1);
	return dup;
}

const char*
slv2_uri_to_path(const char* uri)
{
#ifdef __WIN32__
	if (!strncmp(uri, "file:///", (size_t)8)) {
		return (char*)(uri + 8);
#else
	if (!strncmp(uri, "file://", (size_t)7)) {
		return (char*)(uri + 7);
#endif
	} else {
		return NULL;
	}
}

/** Return the current LANG converted to Turtle (i.e. RFC3066) style.
 * For example, if LANG is set to "en_CA.utf-8", this returns "en-ca".
 */
char*
slv2_get_lang()
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
			SLV2_ERRORF("Illegal LANG `%s' ignored\n", env_lang);
			free(lang);
			return NULL;
		}
	}

	return lang;
}

uint8_t*
slv2_qname_expand(SLV2Plugin p, const char* qname)
{
	const size_t qname_len  = strlen(qname);
	SerdNode     qname_node = { (const uint8_t*)qname,
	                            qname_len + 1, qname_len,
	                            SERD_CURIE };

	SerdChunk uri_prefix;
	SerdChunk uri_suffix;
	if (serd_env_expand(p->world->namespaces, &qname_node, &uri_prefix, &uri_suffix)) {
		const size_t uri_len = uri_prefix.len + uri_suffix.len;
		char*        uri     = malloc(uri_len + 1);
		memcpy(uri,                  uri_prefix.buf, uri_prefix.len);
		memcpy(uri + uri_prefix.len, uri_suffix.buf, uri_suffix.len);
		uri[uri_len] = '\0';
		return (uint8_t*)uri;
	} else {
		SLV2_ERRORF("Failed to expand QName `%s'\n", qname);
		return NULL;
	}
}
