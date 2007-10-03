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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <slv2/port.h>
#include <slv2/types.h>
#include <slv2/util.h>
#include <slv2/values.h>
#include "slv2_internal.h"


/* private */
SLV2Port
slv2_port_new(uint32_t index, const char* symbol/*, const char* node_id*/)
{
	struct _SLV2Port* port = malloc(sizeof(struct _SLV2Port));
	port->index = index;
	port->symbol = strdup(symbol);
	//port->node_id = strdup(node_id);
	return port;
}


/* private */
void
slv2_port_free(SLV2Port port)
{
	free(port->symbol);
	//free(port->node_id);
	free(port);
}


/* private */
SLV2Port
slv2_port_duplicate(SLV2Port port)
{
	SLV2Port result = malloc(sizeof(struct _SLV2Port));
	result->index = port->index;
	result->symbol = strdup(port->symbol);
	//result->node_id = strdup(port->node_id);
	return result;
}


SLV2PortDirection
slv2_port_get_direction(SLV2Plugin p,
                        SLV2Port   port)
{
	SLV2Values direction = slv2_port_get_value(p, port, "rdf:type");

	SLV2PortDirection ret = SLV2_PORT_DIRECTION_UNKNOWN;

	if (!direction)
		return ret;

	for (unsigned i=0; i < slv2_values_size(direction); ++i) {
		SLV2Value val = slv2_values_get_at(direction, i);
		if (slv2_value_is_uri(val)) {
			const char* uri = slv2_value_as_uri(val);
			if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#InputPort"))
				ret = SLV2_PORT_DIRECTION_INPUT;
			else if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#OutputPort"))
				ret = SLV2_PORT_DIRECTION_OUTPUT;
		}
	}
	
	slv2_values_free(direction);

	return ret;
}


SLV2PortDataType
slv2_port_get_data_type(SLV2Plugin p,
                        SLV2Port   port)
{
	SLV2Values type = slv2_port_get_value(p, port, "rdf:type");

	SLV2PortDataType ret = SLV2_PORT_DATA_TYPE_UNKNOWN;

	if (!type)
		return ret;
	
	for (unsigned i=0; i < slv2_values_size(type); ++i) {
		SLV2Value val = slv2_values_get_at(type, i);
		if (slv2_value_is_uri(val)) {
			const char* uri = slv2_value_as_uri(val);
			if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#ControlPort"))
				ret = SLV2_PORT_DATA_TYPE_CONTROL;
			else if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#AudioPort"))
				ret = SLV2_PORT_DATA_TYPE_AUDIO;
			else if (!strcmp(uri, "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"))
				ret = SLV2_PORT_DATA_TYPE_MIDI;
			else if (!strcmp(uri, "http://drobilla.net/ns/lv2ext/osc/0#OSCPort"))
				ret = SLV2_PORT_DATA_TYPE_OSC;
		}
	}

	slv2_values_free(type);

	return ret;
}

#if 0
bool
slv2_port_has_property(SLV2Plugin p,
                       SLV2Port   port,
                       SLV2Value  hint)
{
	/* FIXME: Add SLV2Value QName stuff to make this not suck to use */

	SLV2Values hints = slv2_port_get_value(p, port, "lv2:portHint");

	if (!hints)
		return false;
	
	for (unsigned i=0; i < slv2_values_size(type); ++i) {
		const SLV2Value val = slv2_values_get_at(type, i);
		if (slv2_value_is_uri(val)) {
			const char* uri = slv2_value_as_uri(val);
			if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#connectionOptional"))
				return true;
				ret = SLV2_PORT_DATA_TYPE_CONTROL;
			else if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#AudioPort"))
				ret = SLV2_PORT_DATA_TYPE_AUDIO;
			else if (!strcmp(uri, "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"))
				ret = SLV2_PORT_DATA_TYPE_MIDI;
			else if (!strcmp(uri, "http://drobilla.net/ns/lv2ext/osc/0#OSCPort"))
				ret = SLV2_PORT_DATA_TYPE_OSC;
		}
	}

	slv2_values_free(type);

	return ret;
}
#endif

SLV2Values
slv2_port_get_value(SLV2Plugin  p,
                    SLV2Port    port,
                    const char* property)
{
	assert(property);

	SLV2Values result = NULL;

	char* query = slv2_strjoin(
			"SELECT DISTINCT ?value WHERE {\n"
			"?port lv2:symbol \"", port->symbol, "\";\n\t",
			       property, " ?value .\n}", 0);
			
	result = slv2_plugin_simple_query(p, query, 0);

	free(query);
	
	return result;
}


char*
slv2_port_get_symbol(SLV2Plugin p,
                     SLV2Port   port)
{
	char* symbol = NULL;
	
	SLV2Values result = slv2_port_get_value(p, port, "lv2:symbol");

	if (result && slv2_values_size(result) == 1)
		symbol = strdup(slv2_value_as_string(slv2_values_get_at(result, 0)));
	
	slv2_values_free(result);

	return symbol;
}

	
char*
slv2_port_get_name(SLV2Plugin p,
                   SLV2Port   port)
{
	char* name = NULL;
	
	SLV2Values result = slv2_port_get_value(p, port, "lv2:name");

	if (result && slv2_values_size(result) == 1)
		name = strdup(slv2_value_as_string(slv2_values_get_at(result, 0)));
	
	slv2_values_free(result);

	return name;
}


float
slv2_port_get_default_value(SLV2Plugin p, 
                            SLV2Port   port)
{
	float value = 0.0f;
	
	SLV2Values result = slv2_port_get_value(p, port, "lv2:default");

	if (result && slv2_values_size(result) == 1)
		value = slv2_value_as_float(slv2_values_get_at(result, 0));
	
	slv2_values_free(result);

	return value;
}


float
slv2_port_get_minimum_value(SLV2Plugin p, 
                            SLV2Port   port)
{
	float value = 0.0f;
	
	SLV2Values result = slv2_port_get_value(p, port, "lv2:minimum");

	if (result && slv2_values_size(result) == 1)
		value = slv2_value_as_float(slv2_values_get_at(result, 0));
	
	slv2_values_free(result);

	return value;
}


float
slv2_port_get_maximum_value(SLV2Plugin p, 
                            SLV2Port   port)
{
	float value = 0.0f;
	
	SLV2Values result = slv2_port_get_value(p, port, "lv2:maximum");

	if (result && slv2_values_size(result) == 1)
		value = slv2_value_as_float(slv2_values_get_at(result, 0));
	
	slv2_values_free(result);

	return value;
}


SLV2Values
slv2_port_get_properties(SLV2Plugin p,
                         SLV2Port   port)
{
	return slv2_port_get_value(p, port, "lv2:portProperty");
}

