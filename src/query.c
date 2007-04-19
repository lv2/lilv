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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <librdf.h>
#include <slv2/plugin.h>
#include <slv2/util.h>
#include <slv2/stringlist.h>
#include "private_types.h"


static const char* slv2_query_prefixes =
	"PREFIX rdf:    <http://www.w3.org/1999/02/22-rdf-syntax-ns#>\n"
	"PREFIX rdfs:   <http://www.w3.org/2000/01/rdf-schema#>\n"
	"PREFIX doap:   <http://usefulinc.com/ns/doap#>\n"
	"PREFIX lv2:    <http://lv2plug.in/ontology#>\n";

#if 0
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
#endif

SLV2Strings
slv2_query_get_variable_bindings(librdf_query_results* results,
                                 const char*           variable)
{
	SLV2Strings result = NULL;

    if (librdf_query_results_get_bindings_count(results) > 0)
		result = slv2_strings_new();

    while (!librdf_query_results_finished(results)) {

        librdf_node* node =
            librdf_query_results_get_binding_value_by_name(results, variable);
		
		char* str = NULL;

		switch (librdf_node_get_type(node)) {
		case LIBRDF_NODE_TYPE_RESOURCE:
			str = strdup((const char*)librdf_uri_as_string(librdf_node_get_uri(node)));
			break;
		case LIBRDF_NODE_TYPE_LITERAL:
			str = strdup((const char*)librdf_node_get_literal_value(node));
			break;
		case LIBRDF_NODE_TYPE_BLANK:
			str = strdup((const char*)librdf_node_get_blank_identifier(node));
			break;
		case LIBRDF_NODE_TYPE_UNKNOWN:
		default:
			fprintf(stderr, "Unknown variable binding type for ?%s\n", variable);
			break;
		}
			
		if (str) {
			//printf("?%s = %s\n", variable, str);
			raptor_sequence_push(result, str);
		}

		librdf_free_node(node);

        librdf_query_results_next(results);
    }

    return result;
}


size_t
slv2_query_count_bindings(librdf_query_results* results)
{
	size_t count = 0;

    while (!librdf_query_results_finished(results)) {
		++count;
        librdf_query_results_next(results);
    }

    return count;
}

	
librdf_query_results*
slv2_plugin_query(SLV2Plugin  plugin,
                  const char* sparql_str)
{
	if (!plugin->rdf)
		slv2_plugin_load(plugin);

	librdf_uri* base_uri = plugin->plugin_uri;

	char* query_str = slv2_strjoin(slv2_query_prefixes, sparql_str, NULL);

	//printf("******** Query \n%s********\n", query_str);
	
	librdf_query *rq = librdf_new_query(plugin->model->world, "sparql", NULL,
			(const unsigned char*)query_str, base_uri);
	
	if (!rq) {
		fprintf(stderr, "ERROR: Could not create query\n");
		return NULL;
	}
	
	// Add LV2 ontology to query sources
	//librdf_query_add_data_graph(rq, slv2_ontology_uri, 
	//	NULL, RASQAL_DATA_GRAPH_BACKGROUND);
	
	// Add all plugin data files to query sources
	/*for (unsigned i=0; i < slv2_strings_size(plugin->data_uris); ++i) {
		const char* file_uri_str = slv2_strings_get_at(plugin->data_uris, i);
		raptor_uri* file_uri = raptor_new_uri((const unsigned char*)file_uri_str);
		librdf_query_add_data_graph(rq, file_uri,
			NULL, RASQAL_DATA_GRAPH_BACKGROUND);
		raptor_free_uri(file_uri);
	}*/

	librdf_query_results* results = librdf_query_execute(rq, plugin->rdf);
	
	librdf_free_query(rq);

	free(query_str);

	// FIXME: results leaked internally in places?
	return results;
}


/** Query a single variable */
SLV2Strings
slv2_plugin_simple_query(SLV2Plugin  plugin,
                         const char* sparql_str,
                         const char* variable)
{
	librdf_query_results* results = slv2_plugin_query(plugin, sparql_str);
	SLV2Strings ret = slv2_query_get_variable_bindings(results, variable);
	librdf_free_query_results(results);

	return ret;
}


/** Run a query and count number of matches.
 *
 * More efficient than slv2_plugin_simple_query if you're only interested
 * in the number of results (ie slv2_plugin_num_ports).
 */
unsigned
slv2_plugin_query_count(SLV2Plugin  plugin,
                        const char* sparql_str)
{
	librdf_query_results* results = slv2_plugin_query(plugin, sparql_str);

	if (results) {
		unsigned ret = slv2_query_count_bindings(results);
		librdf_free_query_results(results);
		return ret;
	} else {
		return 0;
	}
}


size_t
slv2_query_count_results(SLV2Plugin  p,
                         const char* query)
{
	char* query_str = slv2_strjoin(slv2_query_prefixes, query, NULL);

	assert(p);
	assert(query_str);

	librdf_query *rq = librdf_new_query(p->model->world, "sparql", NULL,
			(unsigned char*)query_str, NULL);

	//printf("Query: \n%s\n\n", query_str);

	// Add LV2 ontology to query sources
	//librdf_query_add_data_graph(rq, slv2_ontology_uri,
	//	NULL, RASQAL_DATA_GRAPH_BACKGROUND);
	
	librdf_query_results* results = librdf_query_execute(rq, p->model->model);
	assert(results);
	
	size_t count = slv2_query_count_bindings(results);
	
	librdf_free_query_results(results);
	librdf_free_query(rq);
	
	free(query_str);

	return count;
}

