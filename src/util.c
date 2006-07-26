/* LibSLV2
 * Copyright (C) 2006 Dave Robillard <drobilla@connect.carleton.ca>
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

#include <util.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>


void
strappend(char** dst, const char* suffix)
{
	assert(dst);
	assert(*dst);
	assert(suffix);

	const size_t new_length = strlen((char*)*dst) + strlen((char*)suffix) + 1;
	*dst = realloc(*dst, (new_length * sizeof(char)));
	assert(dst);
	strcat((char*)*dst, (char*)suffix);
}


char*
strjoin(const char* first, ...)
{
	size_t  len    = strlen(first);
	char*   result = NULL;
	va_list args;

	va_start(args, first);
	while (1) {
		const char* const s = va_arg(args, const char *);
		if (s == NULL)
			break;
		len += strlen(s);
	}
	va_end(args);

	result = malloc(len + 1);
	if (!result)
		return NULL;

	strcpy(result, first);
	va_start(args, first);
	while (1) {
		const char* const s = va_arg(args, const char *);
		if (s == NULL)
			break;
		strcat(result, s);
	}
	va_end(args);

	return result;
}


/** Convert a URL to a local filesystem path (ie by chopping off the
 * leading "file://".
 *
 * Returns NULL if URL is not a valid URL on the local filesystem.
 * Result is simply a pointer in to \a url and must not be free()'d.
 */
const char*
url2path(const char* const url)
{
	/*assert(strlen((char*)url) > 8);
	char* result = calloc(strlen((char*)url)-7+1, sizeof(char));
	strcpy(result, (char*)url+7);
	return result;*/
	if (!strncmp((char*)url, "file://", (size_t)7))
		return (char*)url + 7;
	else
		return NULL;
}


