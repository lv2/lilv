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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <raptor.h>
#include <slv2/types.h>
#include <slv2/template.h>
#include "slv2_internal.h"


/* private */
SLV2Template
slv2_template_new()
{
	return raptor_new_sequence((void (*)(void*))&slv2_port_signature_free, NULL);
}


/* private */
void
slv2_template_add_port(SLV2Template t)
{
	SLV2PortSignature sig = slv2_port_signature_new(
			SLV2_PORT_DIRECTION_UNKNOWN,
			SLV2_PORT_DATA_TYPE_UNKNOWN);

	raptor_sequence_push(t, sig);
}


/* private */
void slv2_template_port_type(SLV2Template t,
                             uint32_t     port_index,
                             const char*  type_uri)
{
	SLV2PortSignature sig = slv2_template_get_port(t, port_index);

	if (sig) {
		if (!strcmp(type_uri, "http://lv2plug.in/ns/lv2core#InputPort"))
			sig->direction = SLV2_PORT_DIRECTION_INPUT;
		else if (!strcmp(type_uri, "http://lv2plug.in/ns/lv2core#OutputPort"))
			sig->direction = SLV2_PORT_DIRECTION_OUTPUT;
		else if (!strcmp(type_uri, "http://lv2plug.in/ns/lv2core#ControlPort"))
			sig->type = SLV2_PORT_DATA_TYPE_CONTROL;
		else if (!strcmp(type_uri, "http://lv2plug.in/ns/lv2core#AudioPort"))
			sig->type = SLV2_PORT_DATA_TYPE_AUDIO;
		else if (!strcmp(type_uri, "http://ll-plugins.nongnu.org/lv2/ext/MidiPort"))
			sig->type = SLV2_PORT_DATA_TYPE_MIDI;
		else if (!strcmp(type_uri, "http://drobilla.net/ns/lv2ext/osc/0#OSCPort"))
			sig->type = SLV2_PORT_DATA_TYPE_OSC;
	}
}


SLV2PortSignature
slv2_template_get_port(SLV2Template t,
                       uint32_t     index)
{
	if (index > INT_MAX)
		return NULL;
	else
		return (SLV2PortSignature)raptor_sequence_get_at(t, (int)index);
}



void
slv2_template_free(SLV2Template t)
{
	if (t)
		raptor_free_sequence(t);
}


uint32_t
slv2_template_get_num_ports(SLV2Template t)
{
	return raptor_sequence_size(t);
}


uint32_t
slv2_template_get_num_ports_of_type(SLV2Template      t,
                                    SLV2PortDirection direction,
                                    SLV2PortDataType  type)
{
	uint32_t ret = 0;

	for (unsigned i=0; i < slv2_template_get_num_ports(t); ++i) {
		SLV2PortSignature sig = slv2_template_get_port(t, i);
		if (sig->direction == direction && sig->type == type)
			++ret;
	}
	
	return ret;
}

