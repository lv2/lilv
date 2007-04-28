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

#ifndef __SLV2_VALUES_H__
#define __SLV2_VALUES_H__

#include <stdbool.h>
#include <slv2/value.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup values Collections of values
 *
 * SLV2Values is an ordered collection of typed values which is fast for random
 * access by index (i.e. a fancy array).
 *
 * @{
 */


/** Allocate a new, empty SLV2Values
 */
SLV2Values
slv2_values_new();


/** Get the number of elements in a string list.
 */
unsigned
slv2_values_size(SLV2Values list);


/** Get a string from a string list at the given index.
 *
 * @return the element at \a index, or NULL if index is out of range.
 *
 * Time = O(1)
 */
SLV2Value
slv2_values_get_at(SLV2Values list, unsigned index);


/** Return whether \a list contains \a string.
 *
 * Time = O(n)
 */
bool
slv2_values_contains(SLV2Values list, SLV2Value value);


/** Free a string list.
 */
void
slv2_values_free(SLV2Values);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_VALUES_H__ */

