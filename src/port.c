/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define _XOPEN_SOURCE 500

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slv2_internal.h"

SLV2Port
slv2_port_new(SLV2World world, uint32_t index, const char* symbol)
{
	struct _SLV2Port* port = malloc(sizeof(struct _SLV2Port));
	port->index = index;
	port->symbol = slv2_value_new(world, SLV2_VALUE_STRING, symbol);
	port->classes = slv2_values_new();
	return port;
}

void
slv2_port_free(SLV2Port port)
{
	slv2_values_free(port->classes);
	slv2_value_free(port->symbol);
	free(port);
}

SLV2_API
bool
slv2_port_is_a(SLV2Plugin plugin,
               SLV2Port   port,
               SLV2Value  port_class)
{
	SLV2_FOREACH(i, port->classes)
		if (slv2_value_equals(slv2_values_get(port->classes, i), port_class))
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
		SLV2Node  node   = slv2_match_object(ports);
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
	slv2_match_end(ports);
	assert(ret);
	return ret;
}

SLV2_API
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
		p->world->lv2_portproperty_node,
		slv2_value_as_node(property));

	const bool ret = !slv2_matches_end(results);
	slv2_match_end(results);
	return ret;
}

SLV2_API
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
		sord_new_uri(p->world->world, NS_EV "supportsEvent"),
		slv2_value_as_node(event));

	const bool ret = !slv2_matches_end(results);
	slv2_match_end(results);
	return ret;
}

SLV2_API
SLV2Values
slv2_port_get_value_by_qname(SLV2Plugin  p,
                             SLV2Port    port,
                             const char* predicate)
{
	assert(predicate);
	uint8_t* pred_uri = slv2_qname_expand(p, predicate);
	if (!pred_uri) {
		return NULL;
	}

	SLV2Node    port_node = slv2_port_get_node(p, port);
	SLV2Matches results   = slv2_plugin_find_statements(
		p,
		port_node,
		sord_new_uri(p->world->world, pred_uri),
		NULL);

	free(pred_uri);
	return slv2_values_from_stream_objects(p, results);
}

static SLV2Values
slv2_port_get_value_by_node(SLV2Plugin p,
                            SLV2Port   port,
                            SLV2Node   predicate)
{
	assert(sord_node_get_type(predicate) == SORD_URI);

	SLV2Node    port_node = slv2_port_get_node(p, port);
	SLV2Matches results   = slv2_plugin_find_statements(
		p,
		port_node,
		predicate,
		NULL);

	return slv2_values_from_stream_objects(p, results);
}

SLV2_API
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
		slv2_value_as_node(predicate));
}

SLV2_API
SLV2Value
slv2_port_get_symbol(SLV2Plugin p,
                     SLV2Port   port)
{
	return port->symbol;
}

SLV2_API
SLV2Value
slv2_port_get_name(SLV2Plugin p,
                   SLV2Port   port)
{
	SLV2Values results = slv2_port_get_value(p, port,
	                                         p->world->lv2_name_val);

	SLV2Value ret = NULL;
	if (results) {
		SLV2Value val = slv2_values_get_first(results);
		if (slv2_value_is_string(val))
			ret = slv2_value_duplicate(val);
		slv2_values_free(results);
	}

	if (!ret)
		SLV2_WARNF("<%s> has no (mandatory) doap:name\n",
		           slv2_value_as_string(slv2_plugin_get_uri(p)));

	return ret;
}

SLV2_API
SLV2Values
slv2_port_get_classes(SLV2Plugin p,
                      SLV2Port   port)
{
	return port->classes;
}

SLV2_API
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
			? slv2_value_duplicate(slv2_values_get_first(defaults))
			: NULL;
		slv2_values_free(defaults);
	}
	if (min) {
		SLV2Values minimums = slv2_port_get_value_by_node(
			p, port, p->world->lv2_minimum_node);
		*min = minimums
			? slv2_value_duplicate(slv2_values_get_first(minimums))
			: NULL;
		slv2_values_free(minimums);
	}
	if (max) {
		SLV2Values maximums = slv2_port_get_value_by_node(
			p, port, p->world->lv2_maximum_node);
		*max = maximums
			? slv2_value_duplicate(slv2_values_get_first(maximums))
			: NULL;
		slv2_values_free(maximums);
	}
}

SLV2_API
SLV2ScalePoints
slv2_port_get_scale_points(SLV2Plugin p,
                           SLV2Port   port)
{
	SLV2Node    port_node = slv2_port_get_node(p, port);
	SLV2Matches points    = slv2_plugin_find_statements(
		p,
		port_node,
		sord_new_uri(p->world->world, SLV2_NS_LV2 "scalePoint"),
		NULL);

	SLV2ScalePoints ret = NULL;
	if (!slv2_matches_end(points))
		ret = slv2_scale_points_new();

	FOREACH_MATCH(points) {
		SLV2Node point = slv2_match_object(points);

		SLV2Value value = slv2_plugin_get_unique(
			p,
			slv2_node_copy(point),
			slv2_node_copy(p->world->rdf_value_node));

		SLV2Value label = slv2_plugin_get_unique(
			p,
			slv2_node_copy(point),
			slv2_node_copy(p->world->rdfs_label_node));

		if (value && label) {
			slv2_array_append(ret, slv2_scale_point_new(value, label));
		}
	}
	slv2_match_end(points);

	assert(!ret || slv2_values_size(ret) > 0);
	return ret;
}

SLV2_API
SLV2Values
slv2_port_get_properties(SLV2Plugin p,
                         SLV2Port   port)
{
	SLV2Value pred = slv2_value_new_from_node(
		p->world, p->world->lv2_portproperty_node);
	SLV2Values ret = slv2_port_get_value(p, port, pred);
	slv2_value_free(pred);
	return ret;
}

