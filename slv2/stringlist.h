/* SLV2
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef __SLV2_STRINGLIST_H__
#define __SLV2_STRINGLIST_H__

#include <stdbool.h>

typedef void* SLV2Strings;


#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup strings Strings - Collection of strings
 *
 * SLV2Strings is an ordered collection of strings which is fast for random
 * access by index (i.e. a fancy array).
 *
 * @{
 */


/** Allocate a new, empty SLV2Strings
 */
SLV2Strings
slv2_strings_new();


/** Get the number of elements in a string list.
 */
unsigned
slv2_strings_size(const SLV2Strings list);


/** Get a string from a string list at the given index.
 *
 * @return the element at @a index, or NULL if index is out of range.
 */
char*
slv2_strings_get_at(const SLV2Strings list, unsigned index);


/** Return whether @a list contains @a uri.
 */
bool
slv2_strings_contains(const SLV2Strings list, const char* uri);


/** Free a string list. */
void
slv2_strings_free(SLV2Strings);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_STRINGLIST_H__ */

