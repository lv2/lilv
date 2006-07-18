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
ustrappend(uchar** dst, const uchar* suffix)
{
	assert(dst);
	assert(*dst);
	assert(suffix);

	const size_t new_length = strlen((char*)*dst) + strlen((char*)suffix) + 1;
	*dst = realloc(*dst, (new_length * sizeof(char)));
	assert(dst);
	strcat((char*)*dst, (char*)suffix);
}


uchar*
ustrdup(const uchar* src)
{
	assert(src);
	return (uchar*)strdup((char*)src);
}


uchar*
ustrjoin(const uchar* first, ...)
{
	va_list args_list;
	va_start(args_list, first);
	
	uchar* result = vstrjoin(first, args_list);

	va_end(args_list);
	
	return result;
}


uchar*
vstrjoin(const uchar* first, va_list args_list)
{
	// FIXME: this is horribly, awfully, disgracefully slow.
	// so I'm lazy.
	
	const uchar* arg    = NULL;
	uchar*       result = ustrdup(first);

	while ((arg = va_arg(args_list, const uchar*)) != NULL)
		ustrappend(&result, arg);
	
	//va_end(args_list);

	return result;
}



/** Convert a URL to a local filesystem path (ie by chopping off the
 * leading "file://".
 *
 * Returns NULL if URL is not a valid URL on the local filesystem.
 * Result is simply a pointer in to \a url and must not be free()'d.
 */
const char*
url2path(const uchar* const url)
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


