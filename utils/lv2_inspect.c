/* lv2_inspect - Display information about an LV2 plugin.
 * Copyright (C) 2007 Dave Robillard <drobilla.net>
 *  
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <slv2/slv2.h>


void
print_port(SLV2Plugin p, uint32_t index)
{
	SLV2PortID id = slv2_port_by_index(index);

	char* str = NULL;
	SLV2PortClass cl = SLV2_UNKNOWN_PORT_CLASS;

	printf("\n\tPort %d:\n", index);
	
	cl = slv2_port_get_class(p, id);
	printf("\t\tClass: ");
	switch (cl) {
	case SLV2_CONTROL_INPUT:
		printf("Control input");
		break;
	case SLV2_CONTROL_OUTPUT:
		printf("Control output");
		break;
	case SLV2_AUDIO_INPUT:
		printf("Audio input");
		break;
	case SLV2_AUDIO_OUTPUT:
		printf("Audio output");
		break;
	case SLV2_MIDI_INPUT:
		printf("MIDI input");
		break;
	case SLV2_MIDI_OUTPUT:
		printf("MIDI output");
		break;
	case SLV2_UNKNOWN_PORT_CLASS:
		printf("Unknown");
		break;
	}

	printf("\n");
	str = slv2_port_get_symbol(p, id);
	printf("\t\tSymbol: %s\n", str);
	free(str);
	
	str = slv2_port_get_name(p, id);
	printf("\t\tName: %s\n", str);
	free(str);

	if (cl == SLV2_CONTROL_INPUT ||
	    cl == SLV2_CONTROL_OUTPUT) {
		printf("\t\tMinimum: %f\n", slv2_port_get_minimum_value(p, id));
		printf("\t\tMaximum: %f\n", slv2_port_get_maximum_value(p, id));
		printf("\t\tDefault: %f\n", slv2_port_get_default_value(p, id));
	}
}


void
print_plugin(SLV2Plugin p)
{
	char* str = NULL;

	printf("<%s>\n", slv2_plugin_get_uri(p));

	printf("\tData URIs:\n");
	SLV2Strings data_uris = slv2_plugin_get_data_uris(p);
	for (unsigned i=0; i < slv2_strings_size(data_uris); ++i)
		printf("\t\t%s\n", slv2_strings_get_at(data_uris, i));

	printf("\n\tLibrary URI: %s\n\n", slv2_plugin_get_library_uri(p));

	str = slv2_plugin_get_name(p);
	printf("\tName: %s\n", str);
	free(str);

	if (slv2_plugin_has_latency(p))
		printf("\tHas latency: yes\n");
	else
		printf("\tHas latency: no\n");

	printf("\tProperties:\n");
	SLV2Strings v = slv2_plugin_get_properties(p);
	for (unsigned i=0; i < slv2_strings_size(v); ++i)
		printf("\t\t%s\n", slv2_strings_get_at(v, i));
	slv2_strings_free(v);
	
	printf("\tHints:\n");
	v = slv2_plugin_get_hints(p);
	for (unsigned i=0; i < slv2_strings_size(v); ++i)
		printf("\t\t%s\n", slv2_strings_get_at(v, i));
	slv2_strings_free(v);

	uint32_t num_ports = slv2_plugin_get_num_ports(p);
	for (uint32_t i=0; i < num_ports; ++i)
		print_port(p, i);
}


	
int
main(int argc, char** argv)
{
	slv2_init();

	if (argc != 2) {
		fprintf(stderr, "Usage: %s PLUGIN_URI\n", argv[0]);
		return -1;
	}

	SLV2Plugins plugins = slv2_plugins_new();
	slv2_plugins_load_all(plugins);

	SLV2Plugin p = slv2_plugins_get_by_uri(plugins, argv[1]);

	if (!p) {
		fprintf(stderr, "Plugin not found.\n");
		return -1;
	}

	print_plugin(p);

	slv2_finish();

	return 0;
}
