/* SLV2
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
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
#include <assert.h>
#include <librdf.h>
#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include "slv2/types.h"
#include "slv2/collections.h"
#include "slv2/plugin.h"
#include "slv2/query.h"
#include "slv2/util.h"
#include "slv2_internal.h"


static const char* slv2_query_prefixes =
	"PREFIX rdf:    <http://www.w3.org/1999/02/22-rdf-syntax-ns#>\n"
	"PREFIX rdfs:   <http://www.w3.org/2000/01/rdf-schema#>\n"
	"PREFIX doap:   <http://usefulinc.com/ns/doap#>\n"
	"PREFIX foaf:   <http://xmlns.com/foaf/0.1/>\n"
	"PREFIX lv2:    <http://lv2plug.in/ns/lv2core#>\n"
	"PREFIX lv2ev:  <http://lv2plug.in/ns/ext/event#>\n";


/** Create a new SLV2Value from a librdf_node, or return NULL if impossible */
SLV2Value
slv2_value_from_librdf_node(SLV2World world, librdf_node* node)
{
	SLV2Value result = NULL;

	librdf_uri* datatype_uri = NULL;
	SLV2ValueType type = SLV2_VALUE_STRING;

	switch (librdf_node_get_type(node)) {
	case LIBRDF_NODE_TYPE_RESOURCE:
		type = SLV2_VALUE_URI;
		result = slv2_value_new_librdf_uri(world, librdf_node_get_uri(node));
		break;
	case LIBRDF_NODE_TYPE_LITERAL:
		datatype_uri = librdf_node_get_literal_value_datatype_uri(node);
		if (datatype_uri) {
			if (!strcmp((const char*)librdf_uri_as_string(datatype_uri),
						"http://www.w3.org/2001/XMLSchema#integer"))
				type = SLV2_VALUE_INT;
			else if (!strcmp((const char*)librdf_uri_as_string(datatype_uri),
						"http://www.w3.org/2001/XMLSchema#decimal"))
				type = SLV2_VALUE_FLOAT;
			else
				fprintf(stderr, "Unknown datatype %s\n", librdf_uri_as_string(datatype_uri));
		}
		result = slv2_value_new(world, type, (const char*)librdf_node_get_literal_value(node));
		break;
	case LIBRDF_NODE_TYPE_BLANK:
		type = SLV2_VALUE_STRING;
		result = slv2_value_new(world, type, (const char*)librdf_node_get_blank_identifier(node));
		break;
	case LIBRDF_NODE_TYPE_UNKNOWN:
	default:
		fprintf(stderr, "Unknown RDF node type %d\n", librdf_node_get_type(node));
		break;
	}

	return result;
}


SLV2Values
slv2_query_get_variable_bindings(SLV2World   world,
                                 SLV2Results results,
                                 int         variable)
{
	SLV2Values result = NULL;

    if (!librdf_query_results_finished(results->rdf_results))
		result = slv2_values_new();

	while (!librdf_query_results_finished(results->rdf_results)) {
		librdf_node* node = librdf_query_results_get_binding_value(results->rdf_results, variable);

		if (node == NULL) {
			fprintf(stderr, "SLV2 ERROR: Variable %d bound to NULL.\n", variable);
			librdf_query_results_next(results->rdf_results);
			continue;
		}

		SLV2Value val = slv2_value_from_librdf_node(world, node);
		if (val)
			raptor_sequence_push(result, val);

		librdf_free_node(node);
		librdf_query_results_next(results->rdf_results);
	}

    return result;
}


unsigned
slv2_results_size(SLV2Results results)
{
	size_t count = 0;

	while (!slv2_results_finished(results)) {
		++count;
		slv2_results_next(results);
	}

    return count;
}


SLV2Results
slv2_plugin_query_sparql(SLV2Plugin  plugin,
                         const char* sparql_str)
{
	slv2_plugin_load_if_necessary(plugin);

	librdf_uri* base_uri = slv2_value_as_librdf_uri(plugin->plugin_uri);

	char* query_str = slv2_strjoin(slv2_query_prefixes, sparql_str, NULL);

	//printf("******** Query \n%s********\n", query_str);

	librdf_query* query = librdf_new_query(plugin->world->world, "sparql", NULL,
			(const unsigned char*)query_str, base_uri);

	if (!query) {
		fprintf(stderr, "ERROR: Could not create query\n");
		return NULL;
	}

	// FIXME: locale kludges to work around librdf bug
	char* locale = strdup(setlocale(LC_NUMERIC, NULL));

	setlocale(LC_NUMERIC, "POSIX");
	librdf_query_results* results = librdf_query_execute(query, plugin->rdf);
	setlocale(LC_NUMERIC, locale);

	free(locale);

	librdf_free_query(query);
	free(query_str);

	SLV2Results ret = (SLV2Results)malloc(sizeof(struct _SLV2Results));
	ret->world = plugin->world;
	ret->rdf_results = results;

	return ret;
}


void
slv2_results_free(SLV2Results results)
{
	librdf_free_query_results(results->rdf_results);
	free(results);
}


bool
slv2_results_finished(SLV2Results results)
{
	return librdf_query_results_finished(results->rdf_results);
}


SLV2Value
slv2_results_get_binding_value(SLV2Results results, unsigned index)
{
	return slv2_value_from_librdf_node(results->world,
			librdf_query_results_get_binding_value(
					results->rdf_results, index));
}


SLV2Value
slv2_results_get_binding_value_by_name(SLV2Results results, const char* name)
{
	return slv2_value_from_librdf_node(results->world,
			librdf_query_results_get_binding_value_by_name(
					results->rdf_results, name));
}


const char*
slv2_results_get_binding_name(SLV2Results results, unsigned index)
{
	return librdf_query_results_get_binding_name(results->rdf_results, index);
}


void
slv2_results_next(SLV2Results results)
{
	librdf_query_results_next(results->rdf_results);
}


/** Query a single variable */
SLV2Values
slv2_plugin_query_variable(SLV2Plugin  plugin,
                           const char* sparql_str,
                           unsigned    variable)
{
	assert(variable < INT_MAX);

	SLV2Results results = slv2_plugin_query_sparql(plugin, sparql_str);

	SLV2Values ret = slv2_query_get_variable_bindings(plugin->world,
			results, (int)variable);

	slv2_results_free(results);

	return ret;
}


/** Run a query and count number of matches.
 *
 * More efficient than slv2_plugin_simple_query if you're only interested
 * in the number of results (ie slv2_plugin_num_ports).
 *
 * Note the result of this function is probably meaningless unless the query
 * is a SELECT DISTINCT.
 */
unsigned
slv2_plugin_query_count(SLV2Plugin  plugin,
                        const char* sparql_str)
{
	SLV2Results results = slv2_plugin_query_sparql(plugin, sparql_str);

	unsigned ret = 0;

	if (results) {
		ret = slv2_results_size(results);
		slv2_results_free(results);
	}

	return ret;
}

