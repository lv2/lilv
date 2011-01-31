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


static SLV2Node
slv2_port_get_node(SLV2Plugin p,
                   SLV2Port   port)
{
	SLV2Matches ports = slv2_plugin_find_statements(
		p,
		slv2_node_copy(p->plugin_uri->val.uri_val),
		slv2_node_copy(p->world->lv2_port_node),
		NULL);
	SLV2Node ret = NULL;
	FOREACH_MATCH(ports) {
		SLV2Node  node   = MATCH_OBJECT(ports);
		SLV2Value symbol = slv2_plugin_get_unique(
			p,
			slv2_node_copy(node),
			slv2_node_copy(p->world->lv2_symbol_node));

		const bool matches = slv2_value_equals(symbol,
		                                       slv2_port_get_symbol(p, port));

		slv2_value_free(symbol);
		if (matches) {
			ret = slv2_node_copy(node);
			break;
		}
	}
	END_MATCH(ports);
	assert(ret);
	return ret;
}


bool
slv2_port_has_property(SLV2Plugin p,
                       SLV2Port   port,
                       SLV2Value  property)
{
	assert(property);
	SLV2Node    port_node = slv2_port_get_node(p, port);
	SLV2Matches results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, SLV2_NS_LV2 "portProperty"),
		librdf_new_node_from_uri(p->world->world, slv2_value_as_librdf_uri(property)));

	const bool ret = !slv2_matches_end(results);
	END_MATCH(results);
	return ret;
}


bool
slv2_port_supports_event(SLV2Plugin p,
                         SLV2Port   port,
                         SLV2Value  event)
{
#define NS_EV (const uint8_t*)"http://lv2plug.in/ns/ext/event#"

	assert(event);
	SLV2Node    port_node = slv2_port_get_node(p, port);
	SLV2Matches results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, NS_EV "supportsEvent"),
		librdf_new_node_from_uri(p->world->world, slv2_value_as_librdf_uri(event)));

	const bool ret = !slv2_matches_end(results);
	END_MATCH(results);
	return ret;
}


static SLV2Values
slv2_values_from_stream_objects(SLV2Plugin p, SLV2Matches stream)
{
	if (slv2_matches_end(stream)) {
		END_MATCH(stream);
		return NULL;
	}

	SLV2Values values = slv2_values_new();
	FOREACH_MATCH(stream) {
		raptor_sequence_push(
			values,
			slv2_value_new_librdf_node(
				p->world,
				MATCH_OBJECT(stream)));
	}
	END_MATCH(stream);
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

	SLV2Node    port_node = slv2_port_get_node(p, port);
	SLV2Matches results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, (const uint8_t*)pred_uri),
		NULL);

	free(pred_uri);
	return slv2_values_from_stream_objects(p, results);
}


static SLV2Values
slv2_port_get_value_by_node(SLV2Plugin   p,
                            SLV2Port     port,
                            SLV2Node predicate)
{
	assert(librdf_node_is_resource(predicate));

	SLV2Node    port_node = slv2_port_get_node(p, port);
	SLV2Matches results   = slv2_plugin_find_statements(
		p,
		port_node,
		predicate,
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

	return slv2_port_get_value_by_node(
		p, port, 
		librdf_new_node_from_uri(p->world->world,
		                         slv2_value_as_librdf_uri(predicate)));
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

	SLV2Node    port_node = slv2_port_get_node(p, port);
	SLV2Matches results   = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, (const uint8_t*)pred_uri),
		NULL);

	free(pred_uri);
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
	if (def) {
		SLV2Values defaults = slv2_port_get_value_by_node(
			p, port, p->world->lv2_default_node);
		*def = defaults
			? slv2_value_duplicate(slv2_values_get_at(defaults, 0))
			: NULL;
		slv2_values_free(defaults);
	}
	if (min) {
		SLV2Values minimums = slv2_port_get_value_by_node(
			p, port, p->world->lv2_minimum_node);
		*min = minimums
			? slv2_value_duplicate(slv2_values_get_at(minimums, 0))
			: NULL;
		slv2_values_free(minimums);
	}
	if (max) {
		SLV2Values maximums = slv2_port_get_value_by_node(
			p, port, p->world->lv2_maximum_node);
		*max = maximums
			? slv2_value_duplicate(slv2_values_get_at(maximums, 0))
			: NULL;
		slv2_values_free(maximums);
	}
}


SLV2ScalePoints
slv2_port_get_scale_points(SLV2Plugin p,
                           SLV2Port   port)
{
	SLV2Node     port_node = slv2_port_get_node(p, port);
	SLV2Matches  points    = slv2_plugin_find_statements(
		p,
		port_node,
		librdf_new_node_from_uri_string(p->world->world, SLV2_NS_LV2 "scalePoint"),
		NULL);

	SLV2ScalePoints ret = NULL;
	if (!slv2_matches_end(points))
		ret = slv2_scale_points_new();

	FOREACH_MATCH(points) {
		SLV2Node point = MATCH_OBJECT(points);

		SLV2Value value = slv2_plugin_get_unique(
			p,
			slv2_node_copy(point),
			slv2_node_copy(p->world->rdf_value_node));

		SLV2Value label = slv2_plugin_get_unique(
			p,
			slv2_node_copy(point),
			slv2_node_copy(p->world->rdfs_label_node));

		if (value && label) {
			raptor_sequence_push(ret, slv2_scale_point_new(value, label));
		}
	}
	END_MATCH(points);

	assert(!ret || slv2_values_size(ret) > 0);
	return ret;
}



SLV2Values
slv2_port_get_properties(SLV2Plugin p,
                         SLV2Port   port)
{
	return slv2_port_get_value_by_qname(p, port, "lv2:portProperty");
}

