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

#ifndef __UTIL_H
#define __UTIL_H

#include <stdarg.h>
#include <slv2/types.h>

/* Cast a char* to an unsigned char* (Used for string literals) */
#define U(x) (unsigned char*)(x)

/** Append \a suffix to \a *dst, reallocating \a dst as necessary.
 *
 * \a dst will (possibly) be freed, it must be dynamically allocated with malloc
 * or NULL.
 */
void
ustrappend(uchar** dst, const uchar* suffix);

uchar*
ustrdup(const uchar* src);

/** Join all arguments into one string.
 *
 * Arguments are not modified, return value must be free()'d.
 */
uchar*
ustrjoin(const uchar* first, ...);

uchar*
vstrjoin(const uchar* first, va_list args_list);

const char*
url2path(const uchar* const url);


#endif
