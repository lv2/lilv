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


SLV2PortID
slv2_port_by_index(uint32_t index)
{
	SLV2PortID ret;
	ret.is_index = true;
	ret.index = index;
	ret.symbol = NULL;
	return ret;
}


SLV2PortID
slv2_port_by_symbol(const char* symbol)
{
	SLV2PortID ret;
	ret.is_index = false;
	ret.index = UINT_MAX;
	ret.symbol = symbol;
	return ret;
}


SLV2PortClass
slv2_port_get_class(SLV2Plugin p,
                    SLV2PortID id)
{
	SLV2Strings class = slv2_port_get_value(p, id, "rdf:type");

	SLV2PortClass ret = SLV2_UNKNOWN_PORT_CLASS;

	int io = -1; // 0 = in, 1 = out
	enum { UNKNOWN, AUDIO, CONTROL, MIDI } type = UNKNOWN;

	for (unsigned i=0; i < slv2_strings_size(class); ++i) {
		const char* value = slv2_strings_get_at(class, i);
		if (!strcmp(value, "http://lv2plug.in/ontology#InputPort"))
			io = 0;
		else if (!strcmp(value, "http://lv2plug.in/ontology#OutputPort"))
			io = 1;
		else if (!strcmp(value, "http://lv2plug.in/ontology#ControlPort"))
			type = CONTROL;
		else if (!strcmp(value, "http://lv2plug.in/ontology#AudioPort"))
			type = AUDIO;
		else if (!strcmp(value, "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"))
			type = MIDI;
	}

	if (io == 0) {
		if (type == AUDIO)
			ret = SLV2_AUDIO_INPUT;
		else if (type == CONTROL)
			ret = SLV2_CONTROL_INPUT;
		else if (type == MIDI)
			ret = SLV2_MIDI_INPUT;
	} else if (io == 1) {
		if (type == AUDIO)
			ret = SLV2_AUDIO_OUTPUT;
		else if (type == CONTROL)
			ret = SLV2_CONTROL_OUTPUT;
		else if (type == MIDI)
			ret = SLV2_MIDI_OUTPUT;
	}

	slv2_strings_free(class);

	return ret;
}


SLV2Strings
slv2_port_get_value(SLV2Plugin  p,
                    SLV2PortID  id,
                    const char* property)
{
	assert(property);

	SLV2Strings result = NULL;

	if (id.is_index) {
		char index_str[16];
		snprintf(index_str, (size_t)16, "%u", id.index);

		char* query = slv2_strjoin(
				"SELECT DISTINCT ?value WHERE { \n"
				"plugin: lv2:port ?port . \n"
				"?port lv2:index ", index_str, " ;\n\t",
				property, "  ?value . \n}\n", NULL);

		result = slv2_plugin_simple_query(p, query, "value");

		free(query);
	
	} else {

		char* query = slv2_strjoin(
				"SELECT DISTINCT ?value WHERE { \n"
				"plugin: lv2:port ?port . \n"
				"?port lv2:symbol \"", id.symbol, "\" ;\n\t",
				       property, "   ?value . \n}\n", NULL);
		
		result = slv2_plugin_simple_query(p, query, "value");

		free(query);
	}

	return result;
}


char*
slv2_port_get_symbol(SLV2Plugin p,
                     SLV2PortID id)
{
	char* result = NULL;
	
	SLV2Strings prop
		= slv2_port_get_value(p, id, "lv2:symbol");

	if (prop && slv2_strings_size(prop) == 1)
		result = strdup(slv2_strings_get_at(prop, 0));
	
	slv2_strings_free(prop);

	return result;
}

	
char*
slv2_port_get_name(SLV2Plugin p,
                   SLV2PortID id)
{
	char* result = NULL;
	
	SLV2Strings prop
		= slv2_port_get_value(p, id, "lv2:name");

	if (prop && slv2_strings_size(prop) == 1)
		result = strdup(slv2_strings_get_at(prop, 0));
	
	slv2_strings_free(prop);

	return result;
}


float
slv2_port_get_default_value(SLV2Plugin p, 
                            SLV2PortID id)
{
	// FIXME: do casting properly in the SPARQL query
	
	float result = 0.0f;
	
	SLV2Strings prop
		= slv2_port_get_value(p, id, "lv2:default");

	if (prop && slv2_strings_size(prop) == 1)
		result = atof(slv2_strings_get_at(prop, 0));
	
	slv2_strings_free(prop);

	return result;
}


float
slv2_port_get_minimum_value(SLV2Plugin p, 
                            SLV2PortID id)
{
	// FIXME: do casting properly in the SPARQL query
	
	float result = 0.0f;
	
	SLV2Strings prop
		= slv2_port_get_value(p, id, "lv2:minimum");

	if (prop && slv2_strings_size(prop) == 1)
		result = atof(slv2_strings_get_at(prop, 0));
	
	slv2_strings_free(prop);

	return result;
}


float
slv2_port_get_maximum_value(SLV2Plugin p, 
                            SLV2PortID id)
{
	// FIXME: do casting properly in the SPARQL query
	
	float result = 0.0f;
	
	SLV2Strings prop
		= slv2_port_get_value(p, id, "lv2:maximum");

	if (prop && slv2_strings_size(prop) == 1)
		result = atof(slv2_strings_get_at(prop, 0));
	
	slv2_strings_free(prop);

	return result;
}


SLV2Strings
slv2_port_get_properties(SLV2Plugin p,
                         SLV2PortID id)
{
	return slv2_port_get_value(p, id, "lv2:portProperty");
}


SLV2Strings
slv2_port_get_hints(SLV2Plugin p,
                    SLV2PortID id)
{
	return slv2_port_get_value(p, id, "lv2:portHint");
}

