/* SLV2
 * Copyright (C) 2008-2009 Dave Robillard <http://drobilla.net>
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

#ifndef __SLV2_QUERY_H__
#define __SLV2_QUERY_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup slv2_data
 * @{
 */


/** Query a plugin with an arbitrary SPARQL string.
 */
SLV2Results
slv2_plugin_query_sparql(SLV2Plugin  plugin,
                         const char* sparql_str);


/** Free query results.
 */
void
slv2_results_free(SLV2Results results);


/** Return the number of matches in \a results.
 * Note this should not be used to iterate over a result set (since it will
 * iterate to the end of \a results and rewinding is impossible).
 * Instead, use slv2_results_next and slv2_results_finished repeatedly.
 */
unsigned
slv2_results_size(SLV2Results results);


/** Return true iff the end of \a results has been reached.
 */
bool
slv2_results_finished(SLV2Results results);


/** Return a binding in \a results by index.
 * Indices correspond to selected variables in the query in order of appearance.
 * Returned value must be freed by caller with slv2_value_free.
 * \return NULL if binding value can not be expressed as an SLV2Value.
 */
SLV2Value
slv2_results_get_binding_value(SLV2Results results, unsigned index);


/** Return a binding in \a results by name.
 * \a name corresponds to the name of the SPARQL variable (without the '?').
 * Returned value must be freed by caller with slv2_value_free.
 * \return NULL if binding value can not be expressed as an SLV2Value.
 */
SLV2Value
slv2_results_get_binding_value_by_name(SLV2Results results, const char* name);


/** Return the name of a binding in \a results.
 * Returned value is shared and must not be freed by caller.
 * Indices correspond to selected variables in the query in order of appearance.
 */
const char*
slv2_results_get_binding_name(SLV2Results results, unsigned index);


/** Increment \a results to the next match.
 */
void
slv2_results_next(SLV2Results results);


/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __SLV2_QUERY_H__ */
