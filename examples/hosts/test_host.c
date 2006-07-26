/* LibSLV2 Test Host
 * Copyright (C) 2006 Dave Robillard <drobilla@connect.carleton.ca>
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

#define _XOPEN_SOURCE 500

#include <rasqal.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <slv2/types.h>
#include <slv2/plugin.h>
#include <slv2/plugininstance.h>
#include <slv2/pluginlist.h>
#include <slv2/port.h>


void
create_control_input()
{
	printf("Control Input\n");
}


void
create_control_output()
{
	printf("Control Output\n");
}


void
create_audio_input()
{
	printf("Audio Input\n");
}


void
create_audio_output()
{
	printf("Audio Output\n");
}


void
create_port(SLV2Plugin*   plugin,
            SLV2Instance* instance,
            uint32_t      port_index)
{
	enum SLV2PortClass class = slv2_port_get_class(plugin, port_index);

	switch (class) {
	case SLV2_CONTROL_RATE_INPUT:
		create_control_input(port_index);
		break;
	case SLV2_CONTROL_RATE_OUTPUT:
		create_control_output(port_index);
		break;
	case SLV2_AUDIO_RATE_INPUT:

		create_audio_input(port_index);
		break;
	case SLV2_AUDIO_RATE_OUTPUT:
		create_audio_output(port_index);
		break;
	default:
		printf("Unknown port type, ignored.\n");
	}
	//printf("Port %ld class: %d\n", i, slv2_port_get_class(p, i));
}


int
main()
{
	//const char* path = "foo";
	
	const char* path = "/home/dave/code/libslv2/examples/plugins";
	
	SLV2List plugins = slv2_list_new();
	slv2_list_load_path(plugins, path);

	const char* plugin_uri = "http://plugin.org.uk/swh-plugins/amp";
	printf("URI:\t%s\n", plugin_uri);

	const SLV2Plugin* p = slv2_list_get_plugin_by_uri(plugins, plugin_uri);
	if (p) {
		/* Get the plugin's name */
		char* name = slv2_plugin_get_name(p);
		printf("Name:\t%s\n", name);
		free(name);

		uint32_t num_ports = slv2_plugin_get_num_ports(p);
		//printf("Number of ports: %ld\n", num_ports);

		for (uint32_t i=0; i < num_ports; ++i) {
			enum SLV2PortClass class = slv2_port_get_class(p, i);

			switch (class) {
				case SLV2_CONTROL_RATE_INPUT:
					create_control_input(i);
					break;
				case SLV2_CONTROL_RATE_OUTPUT:
					create_control_output(i);
					break;
				case SLV2_AUDIO_RATE_INPUT:
					create_audio_input(i);
					break;
				case SLV2_AUDIO_RATE_OUTPUT:
					create_audio_output(i);
					break;
				default:
					printf("Unknown port type, ignored.\n");
			}
			//printf("Port %ld class: %d\n", i, slv2_port_get_class(p, i));


		}
		
		SLV2Property prop;
		for (uint32_t i=0; i < num_ports; ++i) {
			const char* property = "a";
			prop = slv2_port_get_property(p, i, property);
			if (prop)
				printf("Port %ld %s = %s\n", i, property, prop->values[0]);
			else
				printf("No port %ld %s.\n", i, property);
			free(prop);
		}
		printf("\n");

		SLV2Instance* i = slv2_plugin_instantiate(p, 48000, NULL);
		if (i) {
			printf("Succesfully instantiated plugin.\n");
		
			float gain   = 2.0f;
			float input  = 0.25f;
			float output = 0.0f;
			slv2_instance_connect_port(i, 0, &gain);
			slv2_instance_connect_port(i, 1, &input);
			slv2_instance_connect_port(i, 2, &output);
			
			slv2_instance_activate(i);
			slv2_instance_run(i, 1);
			slv2_instance_deactivate(i);

			printf("Gain: %f, Input: %f  =>  Output: %f\n", gain, input, output);
			slv2_instance_free(i);
		}
	}
	
	slv2_list_free(plugins);

	return 0;
}

