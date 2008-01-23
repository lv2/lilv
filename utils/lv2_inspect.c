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

#include CONFIG_H_PATH
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <slv2/slv2.h>
#include <locale.h>

SLV2Value event_class   = NULL;
SLV2Value control_class = NULL;

void
print_port(SLV2Plugin p, uint32_t index)
{
	SLV2Port port = slv2_plugin_get_port_by_index(p, index);

	char* str = NULL;

	printf("\n\tPort %d:\n", index);

	SLV2Values classes = slv2_port_get_classes(p, port);

	printf("\t\tClasses:\n");
	for (unsigned i=0; i < slv2_values_size(classes); ++i) {
		printf("\t\t\t%s\n", slv2_value_as_uri(slv2_values_get_at(classes, i)));
	}

	if (slv2_port_is_a(p, port, event_class)) {
		SLV2Values supported = slv2_port_get_value(p, port,
			"lv2ev:supportsEvent");
		if (slv2_values_size(supported) > 0) {
			printf("\n\t\tSupported events:\n");
			for (unsigned i=0; i < slv2_values_size(supported); ++i) {
				printf("\t\t\t%s\n", slv2_value_as_uri(slv2_values_get_at(supported, i)));
			}
		}
		slv2_values_free(supported);
	}

	str = slv2_port_get_symbol(p, port);
	printf("\n\t\tSymbol:     %s\n", str);
	free(str);

	str = slv2_port_get_name(p, port);
	printf("\t\tName:       %s\n", str);
	free(str);

	if (slv2_port_is_a(p, port, control_class)) {
		printf("\t\tMinimum:    %f\n", slv2_port_get_minimum_value(p, port));
		printf("\t\tMaximum:    %f\n", slv2_port_get_maximum_value(p, port));
		printf("\t\tDefault:    %f\n", slv2_port_get_default_value(p, port));
	}

	SLV2Values properties = slv2_port_get_properties(p, port);
	printf("\t\tProperties: ");
	for (unsigned i=0; i < slv2_values_size(properties); ++i) {
		if (i > 0) {
			printf("\n\t\t            ");
		}
		printf("%s\n", slv2_value_as_uri(slv2_values_get_at(properties, i)));
	}
	printf("\n");
	slv2_values_free(properties);
}

void
print_plugin(SLV2Plugin p)
{
	char* str = NULL;

	printf("%s\n\n", slv2_plugin_get_uri(p));
	
	str = slv2_plugin_get_name(p);
	printf("\tName:              %s\n", str);
	free(str);
	
	const char* class_label = slv2_plugin_class_get_label(slv2_plugin_get_class(p));
	printf("\tClass:             %s\n", class_label);

	if (slv2_plugin_has_latency(p)) {
		uint32_t latency_port = slv2_plugin_get_latency_port(p);
		printf("\tHas latency:       yes, reported by port %d\n", latency_port);
	} else {
		printf("\tHas latency:       no\n");
	}

	printf("\tBundle:            %s\n", slv2_plugin_get_bundle_uri(p));
	printf("\tBinary:            %s\n", slv2_plugin_get_library_uri(p));

	SLV2UIs uis = slv2_plugin_get_uis(p);
	if (slv2_values_size(uis) > 0) {
		printf("\tGUI:               ");
		for (unsigned i=0; i < slv2_uis_size(uis); ++i) {
			SLV2UI ui = slv2_uis_get_at(uis, i);
			printf("%s\n", slv2_ui_get_uri(ui));

			const char* binary = slv2_ui_get_binary_uri(ui);
			
			SLV2Values types = slv2_ui_get_types(ui);
			for (unsigned i=0; i < slv2_values_size(types); ++i) {
				printf("\t\t\tType:   %s\n", slv2_value_as_uri(slv2_values_get_at(types, i)));
			}
	
			if (binary)
				printf("\t\t\tBinary: %s\n", binary);
	
			printf("\t\t\tBundle: %s\n", slv2_ui_get_bundle_uri(ui));
		}
	}
	slv2_uis_free(uis);

	//SLV2Values ui = slv2_plugin_get_value_for_subject(p,
	//		"<http://ll-plugins.nongnu.org/lv2/ext/gtk2ui#ui>");

	printf("\tData URIs:         ");
	SLV2Values data_uris = slv2_plugin_get_data_uris(p);
	for (unsigned i=0; i < slv2_values_size(data_uris); ++i) {
		if (i > 0) {
			printf("\n\t                   ");
		}
		printf("%s", slv2_value_as_uri(slv2_values_get_at(data_uris, i)));
	}
	printf("\n");


	/* Required Features */

	SLV2Values features = slv2_plugin_get_required_features(p);
	printf("\tRequired Features: ");
	for (unsigned i=0; i < slv2_values_size(features); ++i) {
		if (i > 0) {
			printf("\n\t            ");
		}
		printf("%s\n", slv2_value_as_uri(slv2_values_get_at(features, i)));
	}
	printf("\n");
	slv2_values_free(features);
	
	
	/* Optional Features */

	features = slv2_plugin_get_optional_features(p);
	printf("\tOptional Features: ");
	for (unsigned i=0; i < slv2_values_size(features); ++i) {
		if (i > 0) {
			printf("\n\t            ");
		}
		printf("%s\n", slv2_value_as_uri(slv2_values_get_at(features, i)));
	}
	printf("\n");
	slv2_values_free(features);


	/* Ports */

	const uint32_t num_ports = slv2_plugin_get_num_ports(p);
	
	//printf("\n\t# Ports: %d\n", num_ports);
	
	for (uint32_t i=0; i < num_ports; ++i)
		print_port(p, i);
}


void
print_version()
{
	printf("lv2_inspect (slv2) " PACKAGE_VERSION "\n");
	printf("Copyright (C) 2007 Dave Robillard <dave@drobilla.net>\n");
	printf("License: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}


void
print_usage()
{
	printf("Usage: lv2_inspect PLUGIN_URI\n");
	printf("Show information about an installed LV2 plugin.\n");
}

	
int
main(int argc, char** argv)
{
	setlocale (LC_ALL, "");

	SLV2World world = slv2_world_new();
	slv2_world_load_all(world);

	event_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_EVENT);
	control_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_CONTROL);

	if (argc != 2) {
		print_usage();
		return -1;
	}

	if (!strcmp(argv[1], "--version")) {
		print_version();
		return 0;
	} else if (!strcmp(argv[1], "--help")) {
		print_usage();
		return 0;
	} else if (argv[1][0] == '-') {
		print_usage();
		return -1;
	}

	SLV2Plugins plugins = slv2_world_get_all_plugins(world);

	SLV2Plugin p = slv2_plugins_get_by_uri(plugins, argv[1]);

	if (p) {
		print_plugin(p);
	} else {
		fprintf(stderr, "Plugin not found.\n");
		return -1;
	}

	slv2_plugins_free(world, plugins);
	slv2_world_free(world);

	return (p != NULL ? 0 : -1);
}
