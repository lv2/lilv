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

#include "lilv_internal.h"

LilvPort*
lilv_port_new(LilvWorld* world, uint32_t index, const char* symbol)
{
	LilvPort* port = malloc(sizeof(struct LilvPortImpl));
	port->index = index;
	port->symbol = lilv_value_new(world, LILV_VALUE_STRING, symbol);
	port->classes = lilv_values_new();
	return port;
}

void
lilv_port_free(LilvPort* port)
{
	lilv_values_free(port->classes);
	lilv_value_free(port->symbol);
	free(port);
}

LILV_API
bool
lilv_port_is_a(const LilvPlugin* plugin,
               const LilvPort*   port,
               const LilvValue*  port_class)
{
	LILV_FOREACH(values, i, port->classes)
		if (lilv_value_equals(lilv_values_get(port->classes, i), port_class))
			return true;

	return false;
}

static LilvNode
lilv_port_get_node(const LilvPlugin* p,
                   const LilvPort*   port)
{
	LilvMatches ports = lilv_world_query(
		p->world,
		p->plugin_uri->val.uri_val,
		p->world->lv2_port_node,
		NULL);
	LilvNode ret = NULL;
	FOREACH_MATCH(ports) {
		LilvNode   node   = lilv_match_object(ports);
		LilvValue* symbol = lilv_plugin_get_unique(
			p,
			node,
			p->world->lv2_symbol_node);

		const bool matches = lilv_value_equals(symbol,
		                                       lilv_port_get_symbol(p, port));

		lilv_value_free(symbol);
		if (matches) {
			ret = node;
			break;
		}
	}
	lilv_match_end(ports);
	assert(ret);
	return ret;
}

LILV_API
bool
lilv_port_has_property(const LilvPlugin* p,
                       const LilvPort*   port,
                       const LilvValue*  property)
{
	assert(property);
	LilvNode    port_node = lilv_port_get_node(p, port);
	LilvMatches results   = lilv_world_query(
		p->world,
		port_node,
		p->world->lv2_portproperty_node,
		lilv_value_as_node(property));

	const bool ret = !lilv_matches_end(results);
	lilv_match_end(results);
	return ret;
}

LILV_API
bool
lilv_port_supports_event(const LilvPlugin* p,
                         const LilvPort*   port,
                         const LilvValue*  event)
{
#define NS_EV (const uint8_t*)"http://lv2plug.in/ns/ext/event#"

	assert(event);
	LilvNode    port_node = lilv_port_get_node(p, port);
	LilvMatches results   = lilv_world_query(
		p->world,
		port_node,
		sord_new_uri(p->world->world, NS_EV "supportsEvent"),
		lilv_value_as_node(event));

	const bool ret = !lilv_matches_end(results);
	lilv_match_end(results);
	return ret;
}

static LilvValues*
lilv_port_get_value_by_node(const LilvPlugin* p,
                            const LilvPort*   port,
                            LilvNode          predicate)
{
	assert(sord_node_get_type(predicate) == SORD_URI);

	LilvNode    port_node = lilv_port_get_node(p, port);
	LilvMatches results   = lilv_world_query(
		p->world,
		port_node,
		predicate,
		NULL);

	return lilv_values_from_stream_objects(p->world, results);
}

LILV_API
LilvValues*
lilv_port_get_value(const LilvPlugin* p,
                    const LilvPort*   port,
                    const LilvValue*  predicate)
{
	if ( ! lilv_value_is_uri(predicate)) {
		LILV_ERROR("Predicate is not a URI\n");
		return NULL;
	}

	return lilv_port_get_value_by_node(
		p, port,
		lilv_value_as_node(predicate));
}

LILV_API
const LilvValue*
lilv_port_get_symbol(const LilvPlugin* p,
                     const LilvPort*   port)
{
	return port->symbol;
}

LILV_API
LilvValue*
lilv_port_get_name(const LilvPlugin* p,
                   const LilvPort*   port)
{
	LilvValues* results = lilv_port_get_value(p, port,
	                                          p->world->lv2_name_val);

	LilvValue* ret = NULL;
	if (results) {
		LilvValue* val = lilv_values_get_first(results);
		if (lilv_value_is_string(val))
			ret = lilv_value_duplicate(val);
		lilv_values_free(results);
	}

	if (!ret)
		LILV_WARNF("<%s> has no (mandatory) doap:name\n",
		           lilv_value_as_string(lilv_plugin_get_uri(p)));

	return ret;
}

LILV_API
const LilvValues*
lilv_port_get_classes(const LilvPlugin* p,
                      const LilvPort*   port)
{
	return port->classes;
}

LILV_API
void
lilv_port_get_range(const LilvPlugin* p,
                    const LilvPort*   port,
                    LilvValue**       def,
                    LilvValue**       min,
                    LilvValue**       max)
{
	if (def) {
		LilvValues* defaults = lilv_port_get_value_by_node(
			p, port, p->world->lv2_default_node);
		*def = defaults
			? lilv_value_duplicate(lilv_values_get_first(defaults))
			: NULL;
		lilv_values_free(defaults);
	}
	if (min) {
		LilvValues* minimums = lilv_port_get_value_by_node(
			p, port, p->world->lv2_minimum_node);
		*min = minimums
			? lilv_value_duplicate(lilv_values_get_first(minimums))
			: NULL;
		lilv_values_free(minimums);
	}
	if (max) {
		LilvValues* maximums = lilv_port_get_value_by_node(
			p, port, p->world->lv2_maximum_node);
		*max = maximums
			? lilv_value_duplicate(lilv_values_get_first(maximums))
			: NULL;
		lilv_values_free(maximums);
	}
}

LILV_API
LilvScalePoints*
lilv_port_get_scale_points(const LilvPlugin* p,
                           const LilvPort*   port)
{
	LilvNode    port_node = lilv_port_get_node(p, port);
	LilvMatches points    = lilv_world_query(
		p->world,
		port_node,
		sord_new_uri(p->world->world, (const uint8_t*)LILV_NS_LV2 "scalePoint"),
		NULL);

	LilvScalePoints* ret = NULL;
	if (!lilv_matches_end(points))
		ret = lilv_scale_points_new();

	FOREACH_MATCH(points) {
		LilvNode point = lilv_match_object(points);

		LilvValue* value = lilv_plugin_get_unique(
			p,
			point,
			p->world->rdf_value_node);

		LilvValue* label = lilv_plugin_get_unique(
			p,
			point,
			p->world->rdfs_label_node);

		if (value && label) {
			lilv_array_append(ret, lilv_scale_point_new(value, label));
		}
	}
	lilv_match_end(points);

	assert(!ret || lilv_values_size(ret) > 0);
	return ret;
}

LILV_API
LilvValues*
lilv_port_get_properties(const LilvPlugin* p,
                         const LilvPort*   port)
{
	LilvValue* pred = lilv_value_new_from_node(
		p->world, p->world->lv2_portproperty_node);
	LilvValues* ret = lilv_port_get_value(p, port, pred);
	lilv_value_free(pred);
	return ret;
}
