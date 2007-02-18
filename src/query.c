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

#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <assert.h>
#include <slv2/plugin.h>
#include <slv2/query.h>
#include <slv2/library.h>
#include <slv2/util.h>


char*
slv2_query_header(const SLV2Plugin* p)
{
	const char* const plugin_uri = slv2_plugin_get_uri(p);
	//SLV2URIList files = slv2_plugin_get_data_uris(p);

	char* query_string = slv2_strjoin(
	  "PREFIX rdf:    <http://www.w3.org/1999/02/22-rdf-syntax-ns#>\n"
	  "PREFIX rdfs:   <http://www.w3.org/2000/01/rdf-schema#>\n"
	  "PREFIX doap:   <http://usefulinc.com/ns/doap#>\n"
	  "PREFIX lv2:    <http://lv2plug.in/ontology#>\n"
	  "PREFIX plugin: <", plugin_uri, ">\n", NULL);

	/*for (int i=0; i < slv2_uri_list_size(files); ++i) {
		const char* file_uri = slv2_uri_list_get_at(files, i);
		slv2_strappend(&query_string, "PREFIX data: <");
		slv2_strappend(&query_string, file_uri);
		slv2_strappend(&query_string, ">\n");
	}*/

	slv2_strappend(&query_string, "\n");

	return query_string;
}


char*
slv2_query_lang_filter(const char* variable)
{
	char* result = NULL;
	char* const lang = (char*)getenv("LANG");
	if (lang) {
		// FILTER( LANG(?value) = "en" || LANG(?value) = "" )
		result = slv2_strjoin(
			//"FILTER (lang(?value) = \"", lang, "\"\n"), 0);
			"FILTER( lang(?", variable, ") = \"", lang, 
			"\" || lang(?", variable, ") = \"\" )\n", NULL);
	}

	return result;
}


SLV2Value
slv2_query_get_variable_bindings(rasqal_query_results* results,
                                 const char*           variable)
{
	struct _Value* result = NULL;

    if (rasqal_query_results_get_bindings_count(results) > 0) {
        result = malloc(sizeof(struct _Value));
        result->num_values = 0;
        result->values = NULL;
    }

    while (!rasqal_query_results_finished(results)) {

        rasqal_literal* literal =
            rasqal_query_results_get_binding_value_by_name(results, (const unsigned char*)variable);
        assert(literal != NULL);

        // Add value on to the array, reallocing all the way.
		// Yes, this is disgusting.  Roughly as disgusting as the rasqal query
		// results API.  coincidentally.
        result->num_values++;
        result->values = realloc(result->values, result->num_values * sizeof(char*));
        result->values[result->num_values-1] = strdup((const char*)rasqal_literal_as_string(literal));

        rasqal_query_results_next(results);
    }

    return result;
}


size_t
slv2_query_count_variable_bindings(rasqal_query_results* results)
{
	size_t count = 0;

    while (!rasqal_query_results_finished(results)) {
		++count;
        rasqal_query_results_next(results);
    }

    return count;
}

	
rasqal_query_results*
slv2_plugin_query(SLV2Plugin* plugin,
                  const char* sparql_str)
{
	raptor_uri* base_uri = raptor_new_uri((unsigned char*)slv2_plugin_get_uri(plugin));

	rasqal_query *rq = rasqal_new_query("sparql", NULL);
	
	char* header    = slv2_query_header(plugin);
	char* query_str = slv2_strjoin(header, sparql_str, NULL);

	//printf("Query: \n%s\n\n", query_str);

	rasqal_query_prepare(rq, (unsigned char*)query_str, base_uri);
	
	// Add LV2 ontology to query sources
	rasqal_query_add_data_graph(rq, slv2_ontology_uri, 
		NULL, RASQAL_DATA_GRAPH_BACKGROUND);
	
	// Add all plugin data files to query sources
	for (int i=0; i < slv2_uri_list_size(plugin->data_uris); ++i) {
		const char* file_uri_str = slv2_uri_list_get_at(plugin->data_uris, i);
		raptor_uri* file_uri = raptor_new_uri((const unsigned char*)file_uri_str);
		rasqal_query_add_data_graph(rq, file_uri,
			NULL, RASQAL_DATA_GRAPH_BACKGROUND);
	}

	rasqal_query_results* results = rasqal_query_execute(rq);
	assert(results);
	
	rasqal_free_query(rq);
	raptor_free_uri(base_uri);

	// FIXME: results leaked?
	return results;

	/*
	SLV2Value ret = slv2_query_get_variable_bindings(results, var_name);
	
	rasqal_free_query_results(results);
	rasqal_free_query(rq);

	return ret;*/
}


/** Query a single variable */
SLV2Value
slv2_plugin_simple_query(SLV2Plugin* plugin,
                         const char* sparql_str,
                         const char* variable)
{
	rasqal_query_results* results = slv2_plugin_query(plugin, sparql_str);
	SLV2Value ret = slv2_query_get_variable_bindings(results, variable);
	rasqal_free_query_results(results);

	return ret;
}


// FIXME: stupid interface
size_t
slv2_query_count_results(const SLV2Plugin* p,
                         const char*       query)
{
	char* header    = slv2_query_header(p);
	char* query_str = slv2_strjoin(header, query, NULL);

	assert(p);
	assert(query_str);

	rasqal_query *rq = rasqal_new_query("sparql", NULL);

	//printf("Query: \n%s\n\n", query_str);

	rasqal_query_prepare(rq, (unsigned char*)query_str, NULL);
	
	// Add LV2 ontology to query sources
	rasqal_query_add_data_graph(rq, slv2_ontology_uri,
		NULL, RASQAL_DATA_GRAPH_BACKGROUND);
	
	rasqal_query_results* results = rasqal_query_execute(rq);
	assert(results);
	
	size_t count = slv2_query_count_variable_bindings(results);
	
	rasqal_free_query_results(results);
	rasqal_free_query(rq);
	
	rasqal_finish();
	
	free(query_str);
	free(header);

	return count;
}


/*
size_t
slv2_query_get_num_results(rasqal_query_results* results, const char* var_name)
{
	size_t result = 0;

    while (!rasqal_query_results_finished(results)) {
		if (!strcmp((const char*)rasqal_query_results_get_binding_name(results, 0), var_name)) {
			++result;
		}
        rasqal_query_results_next(results);
	}

	return result;
}
*/

void
slv2_value_free(struct _Value* prop)
{
	for (size_t i=0; i < prop->num_values; ++i)
		free(prop->values[i]);

	free(prop->values);
	free(prop);
}

