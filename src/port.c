/* LibSLV2
 * Copyright (C) 2006 Dave Robillard <drobilla@connect.carleton.ca>
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
#include <string.h>
#include <slv2/port.h>
#include <slv2/types.h>
#include <slv2/query.h>
#include "util.h"

enum SLV2PortClass
slv2_port_get_class(SLV2Plugin*   p,
                    unsigned long index)
{
	struct _Property* class = slv2_port_get_property(p, index, U("rdf:type"));
	assert(class);
	assert(class->num_values == 1);
	assert(class->values);

	if (!strcmp((char*)class->values[0], "http://lv2plug.in/ontology#InputControlRatePort"))
		return SLV2_CONTROL_RATE_INPUT;
	else if (!strcmp((char*)class->values[0], "http://lv2plug.in/ontology#OutputControlRatePort"))
		return SLV2_CONTROL_RATE_OUTPUT;
	else if (!strcmp((char*)class->values[0], "http://lv2plug.in/ontology#InputAudioRatePort"))
		return SLV2_AUDIO_RATE_INPUT;
	else if (!strcmp((char*)class->values[0], "http://lv2plug.in/ontology#OutputAudioRatePort"))
		return SLV2_AUDIO_RATE_OUTPUT;
	else
		return SLV2_UNKNOWN_PORT_CLASS;
}


enum SLV2DataType
slv2_port_get_data_type(SLV2Plugin* p,
                        unsigned long index)
{
	SLV2Property type = slv2_port_get_property(p, index, U("lv2:datatype"));
	assert(type);
	assert(type->num_values == 1);
	assert(type->values);

	if (!strcmp((char*)type->values[0], "http://lv2plug.in/ontology#float"))
		return SLV2_DATA_TYPE_FLOAT;
	else
		return SLV2_UNKNOWN_DATA_TYPE;
}


SLV2Property
slv2_port_get_property(SLV2Plugin* p,
                       unsigned long index,
                       const uchar*  property)
{
	assert(p);
	assert(property);

	char index_str[4];
	snprintf(index_str, (size_t)4, "%lu", index);
	
	rasqal_init();
	
	rasqal_query_results* results = slv2_plugin_run_query(p,
		U("SELECT DISTINCT ?value FROM data: WHERE { \n"
		  "plugin: lv2:port ?port \n"
		  "?port lv2:index "), index_str, U(" \n"
		  "?port "), property, U(" ?value . \n}\n"), NULL);
	
	SLV2Property result = slv2_query_get_results(results);

	rasqal_free_query_results(results);
	rasqal_finish();
	
	return result;
}


uchar*
slv2_port_get_symbol(SLV2Plugin* p, unsigned long index)
{
	// FIXME: leaks
	uchar* result = NULL;
	
	SLV2Property prop
		= slv2_port_get_property(p, index, U("lv2:symbol"));

	if (prop && prop->num_values == 1)
		result = (uchar*)strdup((char*)prop->values[0]);
	free(prop);

	return result;
}


float
slv2_port_get_default_value(SLV2Plugin* p, 
                            unsigned long index)
{
	// FIXME: do casting properly in the SPARQL query
	
	float result = 0.0f;
	
	SLV2Property prop
		= slv2_port_get_property(p, index, U("lv2:default"));

	if (prop && prop->num_values == 1)
		result = atof((char*)prop->values[0]);
	
	return result;
}


float
slv2_port_get_minimum_value(SLV2Plugin* p, 
                            unsigned long index)
{
	// FIXME: do casting properly in the SPARQL query
	
	float result = 0.0f;
	
	SLV2Property prop
		= slv2_port_get_property(p, index, U("lv2:minimum"));

	if (prop && prop->num_values == 1)
		result = atof((char*)prop->values[0]);
	
	return result;
}


float
slv2_port_get_maximum_value(SLV2Plugin* p, 
                            unsigned long index)
{
	// FIXME: do casting properly in the SPARQL query
	
	float result = 0.0f;
	
	SLV2Property prop
		= slv2_port_get_property(p, index, U("lv2:maximum"));

	if (prop && prop->num_values == 1)
		result = atof((char*)prop->values[0]);
	
	return result;
}

