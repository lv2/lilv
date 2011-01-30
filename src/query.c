/* SLV2
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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
#include <redland.h>
#include <limits.h>
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
			SLV2_ERRORF("Variable %d bound to NULL.\n", variable);
			librdf_query_results_next(results->rdf_results);
			continue;
		}

		SLV2Value val = slv2_value_new_librdf_node(world, node);
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

	librdf_query* query = librdf_new_query(plugin->world->world, "sparql", NULL,
			(const uint8_t*)query_str, base_uri);

	if (!query) {
		SLV2_ERRORF("Failed to create query:\n%s", query_str);
		return NULL;
	}

	librdf_query_results* results = librdf_query_execute(query, plugin->rdf);

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
	return slv2_value_new_librdf_node(results->world,
			librdf_query_results_get_binding_value(
					results->rdf_results, index));
}


SLV2Value
slv2_results_get_binding_value_by_name(SLV2Results results, const char* name)
{
	return slv2_value_new_librdf_node(results->world,
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


librdf_stream*
slv2_plugin_find_statements(SLV2Plugin   plugin,
                            librdf_node* subject,
                            librdf_node* predicate,
                            librdf_node* object)
{
	slv2_plugin_load_if_necessary(plugin);
	librdf_statement* q = librdf_new_statement_from_nodes(
		plugin->world->world, subject, predicate, object);
	librdf_stream* results = librdf_model_find_statements(plugin->rdf, q);
	librdf_free_statement(q);
	return results;
}


SLV2Values
slv2_values_from_stream_i18n(SLV2Plugin     p,
                             librdf_stream* stream)
{
	SLV2Values   values  = slv2_values_new();
	librdf_node* nolang  = NULL;
	for (; !librdf_stream_end(stream); librdf_stream_next(stream)) {
		librdf_statement* s     = librdf_stream_get_object(stream);
		librdf_node*      value = librdf_statement_get_object(s);
		if (librdf_node_is_literal(value)) {
			const char* lang = librdf_node_get_literal_value_language(value);
			if (lang) {
				if (!strcmp(lang, slv2_get_lang())) {
					raptor_sequence_push(
						values, slv2_value_new_string(
							p->world, (const char*)librdf_node_get_literal_value(value)));
				}
			} else {
				nolang = value;
			}
		}
		break;
	}
	librdf_free_stream(stream);

	if (slv2_values_size(values) == 0) {
		// No value with a matching language, use untranslated default
		if (nolang) {
			raptor_sequence_push(
				values, slv2_value_new_string(
					p->world, (const char*)librdf_node_get_literal_value(nolang)));
		} else {
			slv2_values_free(values);
			values = NULL;
		}
	}

	return values;
}
