/* lv2_inspect - Display information about an LV2 plugin.
 * Copyright (C) 2007-2009 David Robillard <drobilla.net>
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

#include "slv2-config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include "slv2/slv2.h"

SLV2Value event_class   = NULL;
SLV2Value control_class = NULL;
SLV2Value in_group_pred = NULL;
SLV2Value role_pred     = NULL;
SLV2Value preset_pred   = NULL;
SLV2Value title_pred    = NULL;

void
print_group(SLV2Plugin p, SLV2Value group, SLV2Value type, SLV2Value symbol)
{
	printf("\n\tGroup %s:\n", slv2_value_as_string(group));
	printf("\t\tType: %s\n", slv2_value_as_string(type));
	printf("\t\tSymbol: %s\n", slv2_value_as_string(symbol));
}

void
print_port(SLV2Plugin p, uint32_t index, float* mins, float* maxes, float* defaults)
{
	SLV2Port port = slv2_plugin_get_port_by_index(p, index);

	printf("\n\tPort %d:\n", index);

	if (!port) {
		printf("\t\tERROR: Illegal/nonexistent port\n");
		return;
	}

	SLV2Values classes = slv2_port_get_classes(p, port);

	printf("\t\tType:       ");
	for (unsigned i=0; i < slv2_values_size(classes); ++i) {
		printf("%s", slv2_value_as_uri(slv2_values_get_at(classes, i)));
		if (i != slv2_values_size(classes) - 1)
			printf("\n\t\t            ");
	}

	if (slv2_port_is_a(p, port, event_class)) {
		SLV2Values supported = slv2_port_get_value_by_qname(p, port,
			"lv2ev:supportsEvent");
		if (slv2_values_size(supported) > 0) {
			printf("\n\t\tSupported events:\n");
			for (unsigned i=0; i < slv2_values_size(supported); ++i) {
				printf("\t\t\t%s\n", slv2_value_as_uri(slv2_values_get_at(supported, i)));
			}
		}
		slv2_values_free(supported);
	}

	SLV2ScalePoints points = slv2_port_get_scale_points(p, port);
	if (points)
		printf("\n\t\tScale Points:\n");
	for (unsigned i=0; i < slv2_scale_points_size(points); ++i) {
		SLV2ScalePoint p = slv2_scale_points_get_at(points, i);
		printf("\t\t\t%s = \"%s\"\n",
				slv2_value_as_string(slv2_scale_point_get_value(p)),
				slv2_value_as_string(slv2_scale_point_get_label(p)));
	}
	slv2_scale_points_free(points);

	SLV2Value val = slv2_port_get_symbol(p, port);
	printf("\n\t\tSymbol:     %s\n", slv2_value_as_string(val));

	val = slv2_port_get_name(p, port);
	printf("\t\tName:       %s\n", slv2_value_as_string(val));
	slv2_value_free(val);

	SLV2Values groups = slv2_port_get_value(p, port, in_group_pred);
	if (slv2_values_size(groups) > 0)
		printf("\t\tGroup:      %s\n", slv2_value_as_string(slv2_values_get_at(groups, 0)));
	slv2_values_free(groups);

	SLV2Values roles = slv2_port_get_value(p, port, role_pred);
	if (slv2_values_size(roles) > 0)
		printf("\t\tRole:       %s\n", slv2_value_as_string(slv2_values_get_at(roles, 0)));
	slv2_values_free(roles);

	if (slv2_port_is_a(p, port, control_class)) {
		if (!isnan(mins[index]))
			printf("\t\tMinimum:    %f\n", mins[index]);
		if (!isnan(mins[index]))
			printf("\t\tMaximum:    %f\n", maxes[index]);
		if (!isnan(mins[index]))
			printf("\t\tDefault:    %f\n", defaults[index]);
	}

	SLV2Values properties = slv2_port_get_properties(p, port);
	if (slv2_values_size(properties) > 0)
		printf("\t\tProperties: ");
	for (unsigned i=0; i < slv2_values_size(properties); ++i) {
		if (i > 0) {
			printf("\t\t            ");
		}
		printf("%s\n", slv2_value_as_uri(slv2_values_get_at(properties, i)));
	}
	if (slv2_values_size(properties) > 0)
		printf("\n");
	slv2_values_free(properties);
}

void
print_plugin(SLV2Plugin p)
{
	SLV2Value val = NULL;

	printf("%s\n\n", slv2_value_as_uri(slv2_plugin_get_uri(p)));

	val = slv2_plugin_get_name(p);
	if (val) {
		printf("\tName:              %s\n", slv2_value_as_string(val));
		slv2_value_free(val);
	}

	SLV2Value class_label = slv2_plugin_class_get_label(slv2_plugin_get_class(p));
	if (class_label) {
		printf("\tClass:             %s\n", slv2_value_as_string(class_label));
	}

	val = slv2_plugin_get_author_name(p);
	if (val) {
		printf("\tAuthor:            %s\n", slv2_value_as_string(val));
		slv2_value_free(val);
	}

	val = slv2_plugin_get_author_email(p);
	if (val) {
		printf("\tAuthor Email:      %s\n", slv2_value_as_uri(val));
		slv2_value_free(val);
	}

	val = slv2_plugin_get_author_homepage(p);
	if (val) {
		printf("\tAuthor Homepage:   %s\n", slv2_value_as_uri(val));
		slv2_value_free(val);
	}

	if (slv2_plugin_has_latency(p)) {
		uint32_t latency_port = slv2_plugin_get_latency_port_index(p);
		printf("\tHas latency:       yes, reported by port %d\n", latency_port);
	} else {
		printf("\tHas latency:       no\n");
	}

	printf("\tBundle:            %s\n", slv2_value_as_uri(slv2_plugin_get_bundle_uri(p)));
	printf("\tBinary:            %s\n", slv2_value_as_uri(slv2_plugin_get_library_uri(p)));

	SLV2UIs uis = slv2_plugin_get_uis(p);
	if (slv2_values_size(uis) > 0) {
		printf("\tGUI:               ");
		for (unsigned i=0; i < slv2_uis_size(uis); ++i) {
			SLV2UI ui = slv2_uis_get_at(uis, i);
			printf("%s\n", slv2_value_as_uri(slv2_ui_get_uri(ui)));

			const char* binary = slv2_value_as_uri(slv2_ui_get_binary_uri(ui));

			SLV2Values types = slv2_ui_get_classes(ui);
			for (unsigned i=0; i < slv2_values_size(types); ++i) {
				printf("\t                       Class:  %s\n",
						slv2_value_as_uri(slv2_values_get_at(types, i)));
			}

			if (binary)
				printf("\t                       Binary: %s\n", binary);

			printf("\t                       Bundle: %s\n",
					slv2_value_as_uri(slv2_ui_get_bundle_uri(ui)));
		}
	}
	slv2_uis_free(uis);

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
	if (features)
		printf("\tRequired Features: ");
	for (unsigned i=0; i < slv2_values_size(features); ++i) {
		if (i > 0) {
			printf("\n\t                   ");
		}
		printf("%s", slv2_value_as_uri(slv2_values_get_at(features, i)));
	}
	if (features)
		printf("\n");
	slv2_values_free(features);


	/* Optional Features */

	features = slv2_plugin_get_optional_features(p);
	if (features)
		printf("\tOptional Features: ");
	for (unsigned i=0; i < slv2_values_size(features); ++i) {
		if (i > 0) {
			printf("\n\t                   ");
		}
		printf("%s", slv2_value_as_uri(slv2_values_get_at(features, i)));
	}
	if (features)
		printf("\n");
	slv2_values_free(features);


	/* Presets */

	SLV2Values presets = slv2_plugin_get_value(p, preset_pred);
	if (presets)
		printf("\tPresets: \n");
	for (unsigned i=0; i < slv2_values_size(presets); ++i) {
		SLV2Values titles = slv2_plugin_get_value_for_subject(
			p, slv2_values_get_at(presets, i), title_pred);
		if (titles) {
			SLV2Value title = slv2_values_get_at(titles, 0);
			printf("\t         %s\n", slv2_value_as_string(title));
		}
	}

	
	/* Ports */

	const uint32_t num_ports = slv2_plugin_get_num_ports(p);
	float* mins     = calloc(num_ports, sizeof(float));
	float* maxes    = calloc(num_ports, sizeof(float));
	float* defaults = calloc(num_ports, sizeof(float));
	slv2_plugin_get_port_ranges_float(p, mins, maxes, defaults);

	//printf("\n\t# Ports: %d\n", num_ports);

	for (uint32_t i=0; i < num_ports; ++i)
		print_port(p, i, mins, maxes, defaults);

	free(mins);
	free(maxes);
	free(defaults);
}


void
print_version()
{
	printf("lv2_inspect (slv2) " SLV2_VERSION "\n");
	printf("Copyright (C) 2007-2009 David Robillard <http://drobilla.net>\n");
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
	int ret = 0;
	setlocale (LC_ALL, "");

	SLV2World world = slv2_world_new();
	slv2_world_load_all(world);

	event_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_EVENT);
	control_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_CONTROL);
	in_group_pred = slv2_value_new_uri(world, "http://lv2plug.in/ns/ext/port-groups#inGroup");
	role_pred = slv2_value_new_uri(world, "http://lv2plug.in/ns/ext/port-groups#role");
	preset_pred = slv2_value_new_uri(world, "http://lv2plug.in/ns/dev/presets#hasPreset");
	title_pred = slv2_value_new_uri(world, "http://dublincore.org/documents/dcmi-namespace/title");

	if (argc != 2) {
		print_usage();
		ret = 1;
		goto done;
	}

	if (!strcmp(argv[1], "--version")) {
		print_version();
		ret = 0;
		goto done;
	} else if (!strcmp(argv[1], "--help")) {
		print_usage();
		ret = 0;
		goto done;
	} else if (argv[1][0] == '-') {
		print_usage();
		ret = 2;
		goto done;
	}

	SLV2Plugins plugins = slv2_world_get_all_plugins(world);
	SLV2Value   uri     = slv2_value_new_uri(world, argv[1]);

	SLV2Plugin p = slv2_plugins_get_by_uri(plugins, uri);

	if (p) {
		print_plugin(p);
	} else {
		fprintf(stderr, "Plugin not found.\n");
	}

	ret = (p != NULL ? 0 : -1);

	slv2_value_free(uri);
	slv2_plugins_free(world, plugins);

done:
	slv2_value_free(event_class);
	slv2_value_free(control_class);
	slv2_value_free(in_group_pred);
	slv2_value_free(role_pred);
	slv2_world_free(world);
	return ret;
}

