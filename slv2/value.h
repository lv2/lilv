/* SLV2
 * Copyright (C) 2007-2011 David Robillard <http://drobilla.net>
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

#ifndef __SLV2_VALUE_H__
#define __SLV2_VALUE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "slv2/types.h"

/** @addtogroup slv2_data
 * @{
 */

/** Create a new URI value.
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_uri(SLV2World world, const char* uri);

/** Create a new string value (with no language).
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_string(SLV2World world, const char* str);

/** Create a new integer value.
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_int(SLV2World world, int val);

/** Create a new floating point value.
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_float(SLV2World world, float val);

/** Create a new boolean value.
 * Returned value must be freed by caller with slv2_value_free.
 */
SLV2_API
SLV2Value
slv2_value_new_bool(SLV2World world, bool val);

/** Free an SLV2Value.
 */
SLV2_API
void
slv2_value_free(SLV2Value val);

/** Duplicate an SLV2Value.
 */
SLV2_API
SLV2Value
slv2_value_duplicate(SLV2Value val);

/** Return whether two values are equivalent.
 */
SLV2_API
bool
slv2_value_equals(SLV2Value value, SLV2Value other);

/** Return this value as a Turtle/SPARQL token.
 * <table>
 * <caption>Example Turtle Tokens</caption>
 * <tr><th>URI</th><td>&lt;http://example.org/foo &gt;</td></tr>
 * <tr><th>QName</td><td>doap:name</td></tr>
 * <tr><th>String</td><td>"this is a string"</td></tr>
 * <tr><th>Float</td><td>1.0</td></tr>
 * <tr><th>Integer</td><td>1</td></tr>
 * <tr><th>Boolean</td><td>true</td></tr>
 * </table>
 */
SLV2_API
char*
slv2_value_get_turtle_token(SLV2Value value);

/** Return whether the value is a URI (resource).
 */
SLV2_API
bool
slv2_value_is_uri(SLV2Value value);

/** Return this value as a URI string, e.g. "http://example.org/foo".
 * Valid to call only if slv2_value_is_uri(@a value) returns true.
 * Returned value is owned by @a value and must not be freed by caller.
 */
SLV2_API
const char*
slv2_value_as_uri(SLV2Value value);

/** Return whether the value is a blank node (resource with no URI).
 */
SLV2_API
bool
slv2_value_is_blank(SLV2Value value);

/** Return this value as a blank node identifier, e.g. "genid03".
 * Valid to call only if slv2_value_is_blank(@a value) returns true.
 * Returned value is owned by @a value and must not be freed by caller.
 */
SLV2_API
const char*
slv2_value_as_blank(SLV2Value value);

/** Return whether this value is a literal (i.e. not a URI).
 * Returns true if @a value is a string or numeric value.
 */
SLV2_API
bool
slv2_value_is_literal(SLV2Value value);

/** Return whether this value is a string literal.
 * Returns true if @a value is a string (but not  numeric) value.
 */
SLV2_API
bool
slv2_value_is_string(SLV2Value value);

/** Return @a value as a string.
 */
SLV2_API
const char*
slv2_value_as_string(SLV2Value value);

/** Return whether this value is a decimal literal.
 */
SLV2_API
bool
slv2_value_is_float(SLV2Value value);

/** Return @a value as a float.
 * Valid to call only if slv2_value_is_float(@a value) or
 * slv2_value_is_int(@a value) returns true.
 */
SLV2_API
float
slv2_value_as_float(SLV2Value value);

/** Return whether this value is an integer literal.
 */
SLV2_API
bool
slv2_value_is_int(SLV2Value value);

/** Return @a value as an integer.
 * Valid to call only if slv2_value_is_int(@a value) returns true.
 */
SLV2_API
int
slv2_value_as_int(SLV2Value value);

/** Return whether this value is a boolean.
 */
SLV2_API
bool
slv2_value_is_bool(SLV2Value value);

/** Return @a value as a bool.
 * Valid to call only if slv2_value_is_bool(@a value) returns true.
 */
SLV2_API
bool
slv2_value_as_bool(SLV2Value value);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SLV2_VALUE_H__ */
