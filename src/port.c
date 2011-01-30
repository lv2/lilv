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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "slv2/types.h"
#include "slv2/collections.h"
#include "slv2/port.h"
#include "slv2/query.h"
#include "slv2/util.h"
#include "slv2_internal.h"


/* private */
SLV2Port
slv2_port_new(SLV2World world, uint32_t index, const char* symbol)
{
	struct _SLV2Port* port = malloc(sizeof(struct _SLV2Port));
	port->index = index;
	port->symbol = slv2_value_new(world, SLV2_VALUE_STRING, symbol);
	port->classes = slv2_values_new();
	return port;
}


/* private */
void
slv2_port_free(SLV2Port port)
{
	slv2_values_free(port->classes);
	slv2_value_free(port->symbol);
	free(port);
}


bool
slv2_port_is_a(SLV2Plugin plugin,
               SLV2Port   port,
               SLV2Value  port_class)
{
	for (unsigned i=0; i < slv2_values_size(port->classes); ++i)
		if (slv2_value_equals(slv2_values_get_at(port->classes, i), port_class))
			return true;

	return false;
}


static librdf_node*
slv2_port_get_node(SLV2Plugin p,
                   SLV2Port   port)
{
	librdf_stream* ports = slv2_plugin_find_statements(
		p,
		librdf_new_node_from_uri(p->world->world, p->plugin_uri->val.uri_val),
		librdf_new_node_from_node(p->world->lv2_port_node),
		NULL);
	librdf_node* ret = NULL;
	for (; !librdf_stream_end(ports); librdf_stream_next(ports)) {
		librdf_statement* s    = librdf_stream_get_object(ports);
		librdf_node*      node = librdf_statement_get_object(s);

		SLV2Value symbol = slv2_plugin_get_unique(
			p,
			librdf_new_node_from_node(node),
			librdf_new_node_from_node(p->world->lv2_symbol_node));

		if (slv2_value_equals(symbol, slv2_port_get_symbol(p, port))) {
			ret = librdf_new_node_from_node(node);
			break;
		}
	}
	assert(ret);
	return ret;
}


bool
slv2_port_has_property(SLV2Plugin p,
                       SLV2Port   port,
                       SLV2Value  property)
{
	assert(property);
	librdf_node*   port_node = slv2_port_get_node(p, port);
	librdf_stream* results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, SLV2_NS_LV2 "portProperty"),
		librdf_new_node_from_uri(p->world->world, slv2_value_as_librdf_uri(property)));

	const bool ret = !librdf_stream_end(results);
	librdf_free_stream(results);
	return ret;
}


bool
slv2_port_supports_event(SLV2Plugin p,
                         SLV2Port   port,
                         SLV2Value  event)
{
#define NS_EV (const uint8_t*)"http://lv2plug.in/ns/ext/event#"

	assert(event);
	librdf_node*   port_node = slv2_port_get_node(p, port);
	librdf_stream* results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, NS_EV "supportsEvent"),
		librdf_new_node_from_uri(p->world->world, slv2_value_as_librdf_uri(event)));

	const bool ret = !librdf_stream_end(results);
	librdf_free_stream(results);
	return ret;
}


static SLV2Values
slv2_values_from_stream_objects(SLV2Plugin p, librdf_stream* stream)
{
	if (librdf_stream_end(stream)) {
		return NULL;
	}

	SLV2Values values = slv2_values_new();
	for (; !librdf_stream_end(stream); librdf_stream_next(stream)) {
		raptor_sequence_push(
			values,
			slv2_value_new_librdf_node(
				p->world,
				librdf_statement_get_object(
					librdf_stream_get_object(stream))));
	}
	librdf_free_stream(stream);
	return values;
}


SLV2Values
slv2_port_get_value_by_qname(SLV2Plugin  p,
                             SLV2Port    port,
                             const char* predicate)
{
	assert(predicate);
	char* pred_uri = slv2_qname_expand(p, predicate);
	if (!pred_uri) {
		return NULL;
	}

	librdf_node*   port_node = slv2_port_get_node(p, port);
	librdf_stream* results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, (const uint8_t*)pred_uri),
		NULL);

	return slv2_values_from_stream_objects(p, results);
}


SLV2Values
slv2_port_get_value(SLV2Plugin p,
                    SLV2Port   port,
                    SLV2Value  predicate)
{
	if ( ! slv2_value_is_uri(predicate)) {
		SLV2_ERROR("Predicate is not a URI\n");
		return NULL;
	}

	librdf_node*   port_node = slv2_port_get_node(p, port);
	librdf_stream* results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri(p->world->world, slv2_value_as_librdf_uri(predicate)),
		NULL);

	return slv2_values_from_stream_objects(p, results);
}


SLV2Values
slv2_port_get_value_by_qname_i18n(SLV2Plugin  p,
                                  SLV2Port    port,
                                  const char* predicate)
{
	assert(predicate);
	char* pred_uri = slv2_qname_expand(p, predicate);
	if (!pred_uri) {
		return NULL;
	}

	librdf_node*   port_node = slv2_port_get_node(p, port);
	librdf_stream* results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, (const uint8_t*)pred_uri),
		NULL);

	return slv2_values_from_stream_i18n(p, results);
}


SLV2Value
slv2_port_get_symbol(SLV2Plugin p,
                     SLV2Port   port)
{
	return port->symbol;
}


SLV2Value
slv2_port_get_name(SLV2Plugin p,
                   SLV2Port   port)
{
	SLV2Value  ret     = NULL;
	SLV2Values results = slv2_port_get_value_by_qname_i18n(p, port, "lv2:name");

	if (results && slv2_values_size(results) > 0) {
		ret = slv2_value_duplicate(slv2_values_get_at(results, 0));
	} else {
		results = slv2_port_get_value_by_qname(p, port, "lv2:name");
		if (results && slv2_values_size(results) > 0)
			ret = slv2_value_duplicate(slv2_values_get_at(results, 0));
	}

	slv2_values_free(results);

	return ret;
}


SLV2Values
slv2_port_get_classes(SLV2Plugin p,
                      SLV2Port   port)
{
	return port->classes;
}


void
slv2_port_get_range(SLV2Plugin p,
                    SLV2Port   port,
                    SLV2Value* def,
                    SLV2Value* min,
                    SLV2Value* max)
{
	if (def)
		*def = NULL;
	if (min)
		*min = NULL;
	if (max)
		*max = NULL;

	char* query = slv2_strjoin(
			"SELECT DISTINCT ?def ?min ?max WHERE {\n"
			"<", slv2_value_as_uri(p->plugin_uri), "> lv2:port ?port .\n"
			"?port lv2:symbol \"", slv2_value_as_string(port->symbol), "\".\n",
			"OPTIONAL { ?port lv2:default ?def }\n",
			"OPTIONAL { ?port lv2:minimum ?min }\n",
			"OPTIONAL { ?port lv2:maximum ?max }\n",
			"\n}", NULL);

	SLV2Results results = slv2_plugin_query_sparql(p, query);

    while (!librdf_query_results_finished(results->rdf_results)) {
		librdf_node* def_node = librdf_query_results_get_binding_value(results->rdf_results, 0);
		librdf_node* min_node = librdf_query_results_get_binding_value(results->rdf_results, 1);
		librdf_node* max_node = librdf_query_results_get_binding_value(results->rdf_results, 2);

		if (def && def_node && !*def)
			*def = slv2_value_new_librdf_node(p->world, def_node);
		if (min && min_node && !*min)
			*min = slv2_value_new_librdf_node(p->world, min_node);
		if (max && max_node && !*max)
			*max = slv2_value_new_librdf_node(p->world, max_node);

		if ((!def || *def) && (!min || *min) && (!max || *max))
			break;

		librdf_query_results_next(results->rdf_results);
	}

	slv2_results_free(results);

	free(query);
}


SLV2ScalePoints
slv2_port_get_scale_points(SLV2Plugin p,
                           SLV2Port port)
{
	char* query = slv2_strjoin(
			"SELECT DISTINCT ?value ?label WHERE {\n"
			"<", slv2_value_as_uri(p->plugin_uri), "> lv2:port ?port .\n"
			"?port  lv2:symbol \"", slv2_value_as_string(port->symbol), "\" ;\n",
			"       lv2:scalePoint ?point .\n"
			"?point rdf:value ?value ;\n"
			"       rdfs:label ?label .\n"
			"\n}", NULL);

	SLV2Results results = slv2_plugin_query_sparql(p, query);

	SLV2ScalePoints ret = NULL;

    if (!slv2_results_finished(results))
		ret = slv2_scale_points_new();

    while (!slv2_results_finished(results)) {
		SLV2Value value = slv2_results_get_binding_value(results, 0);
		SLV2Value label = slv2_results_get_binding_value(results, 1);

		if (value && label)
			raptor_sequence_push(ret, slv2_scale_point_new(value, label));

		slv2_results_next(results);
	}

	slv2_results_free(results);

	free(query);

	assert(!ret || slv2_values_size(ret) > 0);

	return ret;
}



SLV2Values
slv2_port_get_properties(SLV2Plugin p,
                         SLV2Port   port)
{
	return slv2_port_get_value_by_qname(p, port, "lv2:portProperty");
}

