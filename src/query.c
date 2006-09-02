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


char*
slv2_query_header(const SLV2Plugin* p)
{
	const char* const plugin_uri = slv2_plugin_get_uri(p);
	const char* const data_file_url = slv2_plugin_get_data_url(p);

	char* query_string = strjoin(
	  "PREFIX rdf:    <http://www.w3.org/1999/02/22-rdf-syntax-ns#> \n"
	  "PREFIX rdfs:   <http://www.w3.org/2000/01/rdf-schema#> \n"
	  "PREFIX doap:   <http://usefulinc.com/ns/doap#> \n"
	  "PREFIX lv2:    <http://lv2plug.in/ontology#> \n"
	  "PREFIX plugin: <", plugin_uri, "> \n",
	  "PREFIX data:   <", data_file_url, "> \n\n", NULL);

	return query_string;
}


char*
slv2_query_lang_filter(const char* variable)
{
	char* result = NULL;
	char* const lang = (char*)getenv("LANG");
	if (lang) {
		// FILTER( LANG(?value) = "en" || LANG(?value) = "" )
		result = strjoin(
			//"FILTER (lang(?value) = \"", lang, "\"\n"), 0);
			"FILTER( lang(?", variable, ") = \"", lang, 
			"\" || lang(?", variable, ") = \"\" )\n", NULL);
	}

	return result;
}


rasqal_query_results*
slv2_plugin_run_query(const SLV2Plugin* p,
                      const char*       query)
{
	char* header    = slv2_query_header(p);
	char* query_str = strjoin(header, query, NULL);

	assert(p);
	assert(query_str);

	rasqal_query *rq = rasqal_new_query("sparql", NULL);

	//printf("Query: \n%s\n\n", query_str);

	rasqal_query_prepare(rq, (unsigned char*)query_str, NULL);
	rasqal_query_results* results = rasqal_query_execute(rq);
	
	rasqal_free_query(rq);
	
	free(query_str);
	free(header);
	
	return results;
}

size_t
slv2_query_get_num_results(rasqal_query_results* results, const char* var_name)
{
	size_t result = 0;

    while (!rasqal_query_results_finished(results)) {
		if (!strcmp(rasqal_query_results_get_binding_name(results, 0), var_name)) {
			++result;
		}
        rasqal_query_results_next(results);
	}

	return result;
}

SLV2Property
slv2_query_get_results(rasqal_query_results* results, const char* var_name)
{
	struct _Property* result = NULL;

    if (rasqal_query_results_get_bindings_count(results) > 0) {
        result = malloc(sizeof(struct _Property));
        result->num_values = 0;
        result->values = NULL;
    }

    while (!rasqal_query_results_finished(results)) {

        rasqal_literal* literal =
            rasqal_query_results_get_binding_value_by_name(results, var_name);
        assert(literal != NULL);

        // Add value on to the array, reallocing all the way.
		// Yes, this is disgusting.  Roughly as disgusting as the rasqal query
		// results API.  coincidentally.
        result->num_values++;
        result->values = realloc(result->values, result->num_values * sizeof(char*));
        result->values[result->num_values-1] = strdup(rasqal_literal_as_string(literal));

        rasqal_query_results_next(results);
    }

    return result;
}

void
slv2_property_free(struct _Property* prop)
{
	for (size_t i=0; i < prop->num_values; ++i)
		free(prop->values[i]);

	free(prop->values);
	free(prop);
}

