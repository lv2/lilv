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

#ifndef __SLV2_QUERY_H__
#define __SLV2_QUERY_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include <rasqal.h>
#include "plugin.h"
#include "types.h"

/** \defgroup query SPARQL query helpers
 *
 * This part is in progress, incomplete, a random mishmash of crap that
 * evolved along with my understanding of this rasqal library.  Nothing
 * to see here, move long now.  Nothing to see here.
 *
 * Eventually this will contain functions that make it convenient for host
 * authors to query plugins in ways libslv2 doesn't nicely wrap (eg. for
 * extensions not (yet) supported by libslv2).
 *
 * @{
 */

/** Return a header for a SPARQL query on the given plugin.
 *
 * The returned header defines the namespace prefixes used in the standard
 * (rdf: rdfs: doap: lv2:), plugin: as the plugin's URI, and data: as the
 * URL of the plugin's RDF (Turtle) data file.
 *
 * Example query to get a plugin's doap:name using this header:
 * 
 * <code>
 * SELECT DISTINCT ?value FROM data: WHERE {
 *     plugin:  doap:name  ?value
 * }
 * </code>
 *
 * \return an unsigned (UTF-8) string which must be free()'d.
 */
char*
slv2_query_header(const SLV2Plugin* p);


/** Return a language filter for the given variable.
 *
 * If the environment variable $LANG is not set, returns NULL.
 *
 * \arg variable SPARQL variable, including "?" or "$" (eg "?value").
 * 
 * This needs to be put inside the WHERE block, after the triples.
 * 
 * eg. FILTER( LANG(?value) = "en" || LANG(?value) = "" )
 */
char*
slv2_query_lang_filter(const char* variable);


/** Run a SPARQL query on a plugin's data file.
 *
 * Header from slv2query_header will be prepended to passed query string (so
 * the default prefixes will be already defined, you don't need to add them
 * yourself).
 *
 * rasqal_init() must be called by the caller before calling this function.
 */
rasqal_query_results*
slv2_plugin_run_query(const SLV2Plugin* p,
                      const char*       query_string);

SLV2Property
slv2_query_get_results(rasqal_query_results* results);

/** Free an SLV2Property. */
void
slv2_property_free(SLV2Property);

/** @} */


#ifdef __cplusplus
}
#endif

#endif /* __SLV2_QUERY_H__ */

