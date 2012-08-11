/*
  Copyright 2012 David Robillard <http://drobilla.net>

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

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lilv/lilv.h"

#include "lilv_config.h"
#include "bench.h"
#include "uri_table.h"

#define BLOCK_LENGTH 512

static LilvNode* lv2_AudioPort   = NULL;
static LilvNode* lv2_CVPort      = NULL;
static LilvNode* lv2_ControlPort = NULL;
static LilvNode* lv2_InputPort   = NULL;
static LilvNode* lv2_OutputPort  = NULL;
static LilvNode* urid_map        = NULL;

static void
print_version(void)
{
	printf(
		"lv2bench (lilv) " LILV_VERSION "\n"
		"Copyright 2012 David Robillard <http://drobilla.net>\n"
		"License: <http://www.opensource.org/licenses/isc-license>\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n");
}

static void
print_usage(void)
{
	printf("Usage: lv2bench\n");
	printf("Benchmark all installed and supported LV2 plugins.\n");
}

static double
bench(const LilvPlugin* p, uint32_t sample_count)
{
	float in[BLOCK_LENGTH];
	float out[BLOCK_LENGTH];

	memset(in, 0, sizeof(in));
	memset(out, 0, sizeof(out));

	URITable uri_table;
	uri_table_init(&uri_table);

	LV2_URID_Map       map           = { &uri_table, uri_table_map };
	LV2_Feature        map_feature   = { LV2_URID_MAP_URI, &map };
	LV2_URID_Unmap     unmap         = { &uri_table, uri_table_unmap };
	LV2_Feature        unmap_feature = { LV2_URID_UNMAP_URI, &unmap };
	const LV2_Feature* features[]    = { &map_feature, &unmap_feature, NULL };

	const char* uri = lilv_node_as_string(lilv_plugin_get_uri(p));
	LilvNodes* required = lilv_plugin_get_required_features(p);
	LILV_FOREACH(nodes, i, required) {
		const LilvNode* feature = lilv_nodes_get(required, i);
		if (!lilv_node_equals(feature, urid_map)) {
			fprintf(stderr, "<%s> requires feature <%s>, skipping\n",
			        uri, lilv_node_as_uri(feature));
			return 0.0;
		}
	}
	
	LilvInstance* instance = lilv_plugin_instantiate(p, 48000.0, features);
	if (!instance) {
		fprintf(stderr, "Failed to instantiate <%s>\n",
		        lilv_node_as_uri(lilv_plugin_get_uri(p)));
		return 0.0;
	}

	float* controls = (float*)calloc(
		lilv_plugin_get_num_ports(p), sizeof(float));
	lilv_plugin_get_port_ranges_float(p, NULL, NULL, controls);

	const uint32_t n_ports = lilv_plugin_get_num_ports(p);
	for (uint32_t index = 0; index < n_ports; ++index) {
		const LilvPort* port = lilv_plugin_get_port_by_index(p, index);
		if (lilv_port_is_a(p, port, lv2_ControlPort)) {
			lilv_instance_connect_port(instance, index, &controls[index]);
		} else if (lilv_port_is_a(p, port, lv2_AudioPort) ||
		           lilv_port_is_a(p, port, lv2_CVPort)) {
			if (lilv_port_is_a(p, port, lv2_InputPort)) {
				lilv_instance_connect_port(instance, index, in);
			} else if (lilv_port_is_a(p, port, lv2_OutputPort)) {
				lilv_instance_connect_port(instance, index, out);
			} else {
				fprintf(stderr, "<%s> port %d neither input nor output, skipping\n",
				        uri, index);
				lilv_instance_free(instance);
				return 0.0;
			}
		} else {
			fprintf(stderr, "<%s> port %d has unknown type, skipping\n",
			        uri, index);
			lilv_instance_free(instance);
			return 0.0;
		}
	}

	lilv_instance_activate(instance);

	struct timespec ts = bench_start();
	for (uint32_t i = 0; i < (sample_count / BLOCK_LENGTH); ++i) {
		lilv_instance_run(instance, BLOCK_LENGTH);
	}
	const double elapsed = bench_end(&ts);

	lilv_instance_deactivate(instance);
	lilv_instance_free(instance);

	uri_table_destroy(&uri_table);

	printf("%lf %s\n", elapsed, uri);
	return elapsed;
}

int
main(int argc, char** argv)
{
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--version")) {
			print_version();
			return 0;
		} else if (!strcmp(argv[i], "--help")) {
			print_usage();
			return 0;
		} else {
			print_usage();
			return 1;
		}
	}

	LilvWorld* world = lilv_world_new();
	lilv_world_load_all(world);

	lv2_AudioPort   = lilv_new_uri(world, LV2_CORE__AudioPort);
	lv2_CVPort      = lilv_new_uri(world, LV2_CORE__CVPort);
	lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);
	lv2_InputPort   = lilv_new_uri(world, LV2_CORE__InputPort);
	lv2_OutputPort  = lilv_new_uri(world, LV2_CORE__OutputPort);
	urid_map        = lilv_new_uri(world, LV2_URID__map);

	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);
	LILV_FOREACH(plugins, i, plugins) {
		bench(lilv_plugins_get(plugins, i), 1 << 19);
	}

	lilv_node_free(urid_map);
	lilv_node_free(lv2_OutputPort);
	lilv_node_free(lv2_InputPort);
	lilv_node_free(lv2_ControlPort);
	lilv_node_free(lv2_CVPort);
	lilv_node_free(lv2_AudioPort);

	lilv_world_free(world);

	return 0;
}
