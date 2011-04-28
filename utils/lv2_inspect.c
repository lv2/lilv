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

#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slv2/slv2.h"

#include "slv2-config.h"

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

	bool first = true;

	SLV2Values classes = slv2_port_get_classes(p, port);
	printf("\t\tType:       ");
	SLV2_FOREACH(values, i, classes) {
		SLV2Value value = slv2_values_get(classes, i);
		if (!first) {
			printf("\n\t\t            ");
		}
		printf("%s", slv2_value_as_uri(value));
		first = false;
	}

	if (slv2_port_is_a(p, port, event_class)) {
		SLV2Values supported = slv2_port_get_value_by_qname(p, port,
			"lv2ev:supportsEvent");
		if (slv2_values_size(supported) > 0) {
			printf("\n\t\tSupported events:\n");
			SLV2_FOREACH(values, i, supported) {
				SLV2Value value = slv2_values_get(supported, i);
				printf("\t\t\t%s\n", slv2_value_as_uri(value));
			}
		}
		slv2_values_free(supported);
	}

	SLV2ScalePoints points = slv2_port_get_scale_points(p, port);
	if (points)
		printf("\n\t\tScale Points:\n");
	SLV2_FOREACH(scale_points, i, points) {
		SLV2ScalePoint p = slv2_scale_points_get(points, i);
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
		printf("\t\tGroup:      %s\n",
		       slv2_value_as_string(
			       slv2_values_get(groups, slv2_values_begin(groups))));
	slv2_values_free(groups);

	SLV2Values roles = slv2_port_get_value(p, port, role_pred);
	if (slv2_values_size(roles) > 0)
		printf("\t\tRole:       %s\n",
		       slv2_value_as_string(
			       slv2_values_get(roles, slv2_values_begin(roles))));
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
	first = true;
	SLV2_FOREACH(values, i, properties) {
		if (!first) {
			printf("\t\t            ");
		}
		printf("%s\n", slv2_value_as_uri(slv2_values_get(properties, i)));
		first = false;
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

	printf("\tBundle:            %s\n",
	       slv2_value_as_uri(slv2_plugin_get_bundle_uri(p)));

	SLV2Value binary_uri = slv2_plugin_get_library_uri(p);
	if (binary_uri) {
		printf("\tBinary:            %s\n",
		       slv2_value_as_uri(slv2_plugin_get_library_uri(p)));
	}

	SLV2UIs uis = slv2_plugin_get_uis(p);
	if (slv2_values_size(uis) > 0) {
		printf("\tUI:                ");
		SLV2_FOREACH(uis, i, uis) {
			SLV2UI ui = slv2_uis_get(uis, i);
			printf("%s\n", slv2_value_as_uri(slv2_ui_get_uri(ui)));

			const char* binary = slv2_value_as_uri(slv2_ui_get_binary_uri(ui));

			SLV2Values types = slv2_ui_get_classes(ui);
			SLV2_FOREACH(values, i, types) {
				printf("\t                       Class:  %s\n",
				       slv2_value_as_uri(slv2_values_get(types, i)));
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
	bool first = true;
	SLV2_FOREACH(values, i, data_uris) {
		if (!first) {
			printf("\n\t                   ");
		}
		printf("%s", slv2_value_as_uri(slv2_values_get(data_uris, i)));
		first = false;
	}
	printf("\n");

	/* Required Features */

	SLV2Values features = slv2_plugin_get_required_features(p);
	if (features)
		printf("\tRequired Features: ");
	first = true;
	SLV2_FOREACH(values, i, features) {
		if (!first) {
			printf("\n\t                   ");
		}
		printf("%s", slv2_value_as_uri(slv2_values_get(features, i)));
		first = false;
	}
	if (features)
		printf("\n");
	slv2_values_free(features);

	/* Optional Features */

	features = slv2_plugin_get_optional_features(p);
	if (features)
		printf("\tOptional Features: ");
	first = true;
	SLV2_FOREACH(values, i, features) {
		if (!first) {
			printf("\n\t                   ");
		}
		printf("%s", slv2_value_as_uri(slv2_values_get(features, i)));
		first = false;
	}
	if (features)
		printf("\n");
	slv2_values_free(features);

	/* Presets */

	SLV2Values presets = slv2_plugin_get_value(p, preset_pred);
	if (presets)
		printf("\tPresets: \n");
	SLV2_FOREACH(values, i, presets) {
		SLV2Values titles = slv2_plugin_get_value_for_subject(
			p, slv2_values_get(presets, i), title_pred);
		if (titles) {
			SLV2Value title = slv2_values_get(titles, slv2_values_begin(titles));
			printf("\t         %s\n", slv2_value_as_string(title));
		}
	}

	/* Ports */

	const uint32_t num_ports = slv2_plugin_get_num_ports(p);
	float* mins     = calloc(num_ports, sizeof(float));
	float* maxes    = calloc(num_ports, sizeof(float));
	float* defaults = calloc(num_ports, sizeof(float));
	slv2_plugin_get_port_ranges_float(p, mins, maxes, defaults);

	for (uint32_t i = 0; i < num_ports; ++i)
		print_port(p, i, mins, maxes, defaults);

	free(mins);
	free(maxes);
	free(defaults);
}

void
print_version()
{
	printf(
		"lv2_inspect (slv2) " SLV2_VERSION "\n"
		"Copyright 2007-2011 David Robillard <http://drobilla.net>\n"
		"License: <http://www.opensource.org/licenses/isc-license>\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n");
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

#define NS_DC   "http://dublincore.org/documents/dcmi-namespace/"
#define NS_PG   "http://lv2plug.in/ns/ext/port-groups#"
#define NS_PSET "http://lv2plug.in/ns/ext/presets#"

	control_class = slv2_value_new_uri(world, SLV2_PORT_CLASS_CONTROL);
	event_class   = slv2_value_new_uri(world, SLV2_PORT_CLASS_EVENT);
	in_group_pred = slv2_value_new_uri(world, NS_PG "inGroup");
	preset_pred   = slv2_value_new_uri(world, NS_PSET "hasPreset");
	role_pred     = slv2_value_new_uri(world, NS_PG "role");
	title_pred    = slv2_value_new_uri(world, NS_DC "title");

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

done:
	slv2_value_free(title_pred);
	slv2_value_free(role_pred);
	slv2_value_free(preset_pred);
	slv2_value_free(in_group_pred);
	slv2_value_free(event_class);
	slv2_value_free(control_class);
	slv2_world_free(world);
	return ret;
}

