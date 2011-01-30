/* SLV2
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "slv2/util.h"
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


const char*
slv2_uri_to_path(const char* uri)
{
	if (!strncmp(uri, "file://", (size_t)7))
		return (char*)(uri + 7);
	else
		return NULL;
}


const char*
slv2_get_lang()
{
	static char lang[32];
	lang[31] = '\0';
	char* tmp = getenv("LANG");
	if (!tmp) {
		lang[0] = '\0';
	} else {
		strncpy(lang, tmp, 31);
		for (int i = 0; i < 31 && lang[i]; ++i) {
			if (lang[i] == '_') {
				lang[i] = '-';
			} else if (   !(lang[i] >= 'a' && lang[i] <= 'z')
			           && !(lang[i] >= 'A' && lang[i] <= 'Z')) {
				lang[i] = '\0';
				break;
			}
		}
	}

	return lang;
}


char*
slv2_qname_expand(SLV2Plugin p, const char* qname)
{
	char* colon = strchr(qname, ':');
	if (!colon || colon == qname) {
		SLV2_ERRORF("Invalid QName `%s'\n", qname);
		return NULL;
	}

	const size_t prefix_len = colon - qname;
	char*        prefix     = malloc(prefix_len + 1);
	memcpy(prefix, qname, prefix_len);
	prefix[prefix_len] = '\0';

	char* namespace = librdf_hash_get(p->world->namespaces, prefix);
	free(prefix);
	if (!namespace) {
		SLV2_ERRORF("QName `%s' has Undefined prefix\n", qname);
		return NULL;
	}

	const size_t qname_len     = strlen(qname);
	const size_t suffix_len    = qname_len - prefix_len - 1;
	const size_t namespace_len = strlen(namespace);
	char*        uri           = malloc(namespace_len + suffix_len + 1);
	memcpy(uri, namespace, namespace_len);
	memcpy(uri + namespace_len, colon + 1, qname_len - prefix_len - 1);
	uri[namespace_len + suffix_len] = '\0';

	free(namespace);
	return uri;
}
