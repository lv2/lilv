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

#include <util.h>
#include <stdlib.h>
#include <assert.h>
#include <slv2/plugin.h>
#include <slv2/query.h>


unsigned char*
slv2_query_header(const SLV2Plugin* p)
{
	const unsigned char* plugin_uri = slv2_plugin_get_uri(p);
	const unsigned char* data_file_url = slv2_plugin_get_data_url(p);

	unsigned char* query_string = ustrjoin(U(
		  "PREFIX rdf:    <http://www.w3.org/1999/02/22-rdf-syntax-ns#> \n"
		  "PREFIX rdfs:   <http://www.w3.org/2000/01/rdf-schema#> \n"
		  "PREFIX doap:   <http://usefulinc.com/ns/doap#> \n"
		  "PREFIX lv2:    <http://lv2plug.in/ontology#> \n"
		  "PREFIX plugin: <"), plugin_uri, U("> \n"),
		U("PREFIX data:   <"), data_file_url, U("> \n\n"), NULL);

	return query_string;
}


unsigned char*
slv2_query_lang_filter(const uchar* variable)
{
	uchar* result = NULL;
	uchar* const lang = (uchar*)getenv("LANG");
	if (lang) {
		// FILTER( LANG(?value) = "en" || LANG(?value) = "" )
		result = ustrjoin(
			//U("FILTER (lang(?value) = \""), lang, U("\")\n"), 0);
			U("FILTER( lang(?value) = \""), lang, 
			U("\" || lang(?value) = \"\" )\n"), NULL);
	}

	return result;
}


rasqal_query_results*
slv2_plugin_run_query(const SLV2Plugin* p,
                      const uchar*        first, ...)
{

	/* FIXME:  Too much unecessary allocation */
	uchar* header = slv2_query_header(p);
	
	va_list args_list;
	va_start(args_list, first);

	uchar* args_str = vstrjoin(first, args_list);
	uchar* query_str = ustrjoin(header, args_str, NULL);
	va_end(args_list);

	assert(p);
	assert(query_str);

	rasqal_query *rq = rasqal_new_query("sparql", NULL);

	//printf("Query: \n%s\n\n", query_str);

	rasqal_query_prepare(rq, query_str, NULL);
	rasqal_query_results* results = rasqal_query_execute(rq);
	
	rasqal_free_query(rq);
	
	free(query_str);
	free(args_str);
	free(header);
	
	return results;
}


SLV2Property
slv2_query_get_results(rasqal_query_results* results)
{
	struct _Property* result = NULL;
	
	if (rasqal_query_results_get_count(results) > 0) {
		result = malloc(sizeof(struct _Property));
		result->num_values = 0;
		result->values = NULL;
	}
	
	while (!rasqal_query_results_finished(results)) {
		
		rasqal_literal* literal =
			rasqal_query_results_get_binding_value_by_name(results, U("value"));
		assert(literal != NULL);
		
		// Add value on to the array.  Yes, this is disgusting.
		result->num_values++;
		// FIXME LEAK:
		result->values = realloc(result->values, result->num_values * sizeof(char*));
		result->values[result->num_values-1] = ustrdup(rasqal_literal_as_string(literal));
		
		rasqal_query_results_next(results);
	}

	return result;
}

