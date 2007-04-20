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

#ifndef __SLV2_CATEGORIES_H__
#define __SLV2_CATEGORIES_H__

#include <slv2/category.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void* SLV2Categories;


/** \defgroup categories Categories
 * 
 * @{
 */

/** Get the number of plugins in the list.
 */
unsigned
slv2_categories_size(SLV2Categories list);


/** Get a category from the list by URI.
 *
 * Return value is shared (stored in \a list) and must not be freed or
 * modified by the caller in any way.
 *
 * Time = O(log2(n))
 * 
 * \return NULL if plugin with \a url not found in \a list.
 */
SLV2Category
slv2_categories_get_by_uri(SLV2Categories list,
                           const char*    uri);


/** Get a plugin from the list by index.
 *
 * \a index has no significance other than as an index into this list.
 * Any \a index not less than slv2_categories_get_length(list) will return NULL,
 * so all categories in a list can be enumerated by repeated calls
 * to this function starting with \a index = 0.
 *
 * Time = O(1)
 *
 * \return NULL if \a index out of range.
 */
SLV2Category
slv2_categories_get_at(SLV2Categories list,
                       unsigned       index);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __SLV2_CATEGORIES_H__ */

